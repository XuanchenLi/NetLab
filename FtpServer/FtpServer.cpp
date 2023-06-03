/*
* @File: FTPServer.cpp
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/29 下午 5:06
* @Description: 声明了FTP服务端所需的各种方法
* Version V1.0
*/
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <io.h>
#include <filesystem>
#include <fstream>
#include "FTPServer.h"

/**
* @brief	代表终止线程的指令，线程接收到该消息时结束运行
*/
const ULONG_PTR FTPServer::COMPLETION_KEY_SHUTDOWN = (ULONG_PTR)-1;

/**
* @brief	服务器对象构造函数
* @param	pDic	当前工作目录
* @param	pPort	服务器端口
*/
FTPServer::FTPServer(std::string pDic, u_short pPort) : dic(pDic)
{
	/*初始化WSA*/
	WSADATA wsaData;
	SYSTEM_INFO sysInfo;
	puts("初始化WSA...");
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("初始化WSA失败");
	}
	/*创建完成端口*/
	puts("创建完成端口...");
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hComPort == NULL)
	{
		throw std::runtime_error("创建完成端口失败");
	}
	/*创建工作线程*/
	GetSystemInfo(&sysInfo);
	HANDLE wtHandle;
	puts("创建工作线程...");
	for (int i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
	{
		wtHandle = (HANDLE)_beginthreadex(NULL, 0, FTPServer::ftpThreadMain, (LPVOID)this, 0, NULL);
		if (wtHandle == NULL)
		{
			throw std::runtime_error("创建工作线程失败");
		}
		workerHandles.push_back(wtHandle);
	}
	/*创建服务器套接字*/
	puts("创建套接字...");
	hServerSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hServerSock == INVALID_SOCKET)
	{
		throw std::runtime_error("创建WSA套接字失败:" + WSAGetLastError());
	}
	/*绑定套接字*/
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(pPort);
	puts("绑定套接字...");
	if (bind(hServerSock, (SOCKADDR*)&servAddr, sizeof(servAddr)))
	{
		throw std::runtime_error("绑定套接字失败" + WSAGetLastError());
	}
	puts("开始监听...");
	listen(hServerSock, 5);
	int recvBytes, flags = 0;
	puts("FTP服务器启动成功");
	/*服务器循环接收客户端连接*/
	while (true)
	{
		/*接收连接，初始化客户端句柄对象*/
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);
		SOCKET hClntSock = accept(hServerSock, (SOCKADDR*)&clntAddr, &addrLen);
		clntSocks.insert(hClntSock);
		ClntHandleData* handleInfo = (ClntHandleData*)malloc(sizeof(ClntHandleData));
		handleInfo->clntSock = hClntSock;
		memcpy(&(handleInfo->clntAddr), &clntAddr, addrLen);
		printf("客户端[%s:%d]连接成功\n", inet_ntoa(*((in_addr*)&(handleInfo->clntAddr.sin_addr))), ntohs(handleInfo->clntAddr.sin_port));
		/*绑定客户端套接字到完成端口对象*/
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);
		/*创建IO数据对象用户异步接收客户端消息*/
		IOData* ioInfo = (IOData*)malloc(sizeof(IOData));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = RW_MODE::READ;
		WSARecv(handleInfo->clntSock, &(ioInfo->wsaBuf), 1, (LPDWORD)&recvBytes, (LPDWORD)&flags, &(ioInfo->overlapped), NULL);
	}
}
/**
* @brief	服务器对象析构函数，释放所有资源
*/
FTPServer::~FTPServer()
{
	// 向每个线程发送一个关闭信号。关闭信号是一个特殊的完成键。
	for (size_t i = 0; i < workerHandles.size(); ++i)
	{
		PostQueuedCompletionStatus(hComPort, 0, COMPLETION_KEY_SHUTDOWN, NULL);
	}

	// 等待所有工作线程关闭。
	WaitForMultipleObjects(static_cast<DWORD>(workerHandles.size()), &workerHandles[0], TRUE, INFINITE);

	// 关闭完成端口。
	CloseHandle(hComPort);

	// 关闭线程句柄。
	for (size_t i = 0; i < workerHandles.size(); ++i)
	{
		CloseHandle(workerHandles[i]);
	}
	// 关闭客户端套接字
	for (auto sock : clntSocks)
	{
		closesocket(sock);
	}
	/*关闭服务器套接字*/
	closesocket(hServerSock);
	/*清除WSA环境*/
	WSACleanup();
}
/**
* @brief	处理LIST指令
* @param	ClntHandleData*	客户端句柄对象指针
* @param	IOData*			IO数据对象指针
* @param	string			目录路径
*
*/
void FTPServer::handleList(ClntHandleData* cHandleData, IOData* ioData, std::string dic)
{
	/*获取文件信息*/
	auto fileNameList = getFileInfoList(dic);
	/*传输文件信息*/
	transList(fileNameList, *cHandleData);
	/*异步接收下次用户发送的消息*/
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandleData->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}
/**
* @brief	处理QUIT指令
* @param	ClntHandleData*				客户端句柄对象指针
* @param	IOData*						IO数据对象指针
* @param	std::unordered_set<SOCKET>	FTP服务器维护的客户端句柄集合
*
*/
void FTPServer::handleQuit(ClntHandleData* cHandleData, IOData* ioData, std::unordered_set<SOCKET>&clntSocks)
{
	/*释放资源*/
	free(ioData);
	clntSocks.erase(cHandleData->clntSock);
	closesocket(cHandleData->clntSock);
	printf("客户端[%s:%d]断开连接\n", inet_ntoa(*((in_addr*)&(cHandleData->clntAddr.sin_addr))), ntohs(cHandleData->clntAddr.sin_port));
}
/**
* @brief	处理未识别的指令
* @param	string						目录路径
*
* @return	std::vector<_finddata_t>	文件信息列表
*/
void FTPServer::handleUnknown(ClntHandleData* cHandleData, IOData* ioData)
{
	/*发送提示消息*/
	IOData* sendData = getNewBlock(RW_MODE::WRITE);
	sprintf(sendData->buffer, "ERROR: Unknown Command.");
	sendData->wsaBuf.len = strlen(sendData->wsaBuf.buf);
	WSASend(cHandleData->clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
	/*接收下一次消息*/
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandleData->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}
/**
* @brief	获取文件信息列表
* @param	string						目录路径
*
* @return	std::vector<_finddata_t>	文件信息列表
*/
std::vector<_finddata_t> FTPServer::getFileInfoList(std::string dic)
{
	std::vector<_finddata_t> res;
	long hFile = 0;
	struct _finddata_t fileinfo;
	std::string p;
	/*遍历目录中的所有非文件夹文件*/
	if ((hFile = _findfirst(p.assign(dic).append("*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			if (!(fileinfo.attrib & _A_SUBDIR))
			{
				res.push_back(fileinfo);
			}
		} while (_findnext(hFile, &fileinfo) == 0);	//返回值位0代表还有下个文件
		_findclose(hFile);	// 关闭句柄
	} 
	return res;
}
/**
* @brief	判断文件是否可用
* @param	string						文件路径
*
* @return	bool						True代表可用
*/
bool FTPServer::isFileAvailable(std::string filename)
{
	std::filesystem::path fName(filename);
	/*判断文件存在*/
	if (!std::filesystem::exists(fName))
	{
		throw std::exception("文件不存在");
	}
	/*判断是否为目录*/
	if (std::filesystem::is_directory(fName))
	{
		throw std::exception("不支持传输文件夹");
	}
	/*判断文件状态和权限信息*/
	if (std::filesystem::is_regular_file(fName) && std::filesystem::status_known(std::filesystem::status(fName)))
	{
		if ((std::filesystem::status(fName).permissions() & std::filesystem::perms::others_read) != std::filesystem::perms::none)
		{
			return true;
		}
		else
		{
			throw std::exception("文件不可读");
		}
	}
	else
	{
		throw std::exception("不支持的文件类型");
	}
	return false;
}
/**
* @brief	从缓冲区中读取用户发送的指令和参数
* @param	IOData		IO数据对象
*
* @return	std::pair<FTP_COMMAND, std::string>	指令和参数
*/
std::pair<FTP_COMMAND, std::string> FTPServer::readCommand(const IOData* ioData)
{
	std::stringstream is(ioData->buffer);	//字符串流对象
	std::pair<FTP_COMMAND, std::string> res;
	std::string sCmd;
	is >> sCmd;
	if (sCmd == "QUIT")
	{
		res.first = FTP_COMMAND::QUIT;
	}
	else if (sCmd == "GET")
	{
		res.first = FTP_COMMAND::GET;
		is >> res.second;
	}
	else if (sCmd == "PUT")
	{
		res.first = FTP_COMMAND::PUT;
		is >> res.second;
	}
	else if (sCmd == "LIST")
	{
		res.first = FTP_COMMAND::LIST;
	}
	else
	{
		res.first = FTP_COMMAND::UNKNOWN;
	}
	return res;
}
/**
* @brief	IOCP工作线程主函数
* @param	serverPtr_		服务器对象指针
*
* @return	DWORD32
*/
DWORD32 WINAPI FTPServer::ftpThreadMain(LPVOID serverPtr_)
{
	/*通过服务器对象获取完成端口*/
	HANDLE hComPort = ((FTPServer*)serverPtr_)->hComPort;
	IOData* ioInfo;
	ClntHandleData* handleInfo;
	DWORD bytesTrans;
	DWORD flags = 0;

	/*工作线程循环处理IO完成消息*/
	while (true)
	{
		/*等待并获取已完成的IO消息，如果没有则阻塞*/
		boolean result = GetQueuedCompletionStatus(hComPort, &bytesTrans, (PULONG_PTR)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		if (result)
		{
			/*如果是终止KEY则终止运行*/
			if ((ULONG_PTR)handleInfo == FTPServer::COMPLETION_KEY_SHUTDOWN)
			{
				break;
			}
			SOCKET clntSock = handleInfo->clntSock;
			if (ioInfo->rwMode == RW_MODE::READ)	/*处理完成的读操作*/
			{

				if (bytesTrans == 0)
				{
					continue;
				}
				ioInfo->buffer[ioInfo->wsaBuf.len] = '\0';
				/*判断命令类型，转到相应处理函数*/
				auto cmd = readCommand(ioInfo);
				printf("[%s:%d]: %s", inet_ntoa(*((in_addr*)&(handleInfo->clntAddr.sin_addr))), ntohs(handleInfo->clntAddr.sin_port), ioInfo->wsaBuf.buf);
				switch (cmd.first)
				{
				case::FTP_COMMAND::GET:
					handleGet(handleInfo, ioInfo, ((FTPServer*)serverPtr_)->dic + cmd.second);
					break;
				case::FTP_COMMAND::PUT:
					handlePut(handleInfo, ioInfo, ((FTPServer*)serverPtr_)->dic + cmd.second);
					break;
				case::FTP_COMMAND::LIST:
					handleList(handleInfo, ioInfo, ((FTPServer*)serverPtr_)->dic);
					break;
				case::FTP_COMMAND::QUIT:
					handleQuit(handleInfo, ioInfo, ((FTPServer*)serverPtr_)->clntSocks);
					break;
				case::FTP_COMMAND::UNKNOWN:
					handleUnknown(handleInfo, ioInfo);
					break;
				}
				
			}
			else if (ioInfo->rwMode == RW_MODE::WRITE)	/*处理完成的写操作*/
			{
				// 释放IO数据资源
				free(ioInfo);
			}

		}
	}
	return DWORD32();
}
/**
	* @brief	封装传输文件的逻辑
	* @param	fName			文件名
	* @param	ClntHandleData	客户端句柄对象
	*
	*/
void FTPServer::transFile(std::string fName, const ClntHandleData& cHandle)
{
	// 读方式打开文件
	std::ifstream input(fName.c_str(), std::ios::binary);
	// 检查文件是否正确打开
	if (!input)
	{
		throw std::exception("打开文件失败");
	}
	// 获取文件大小
	input.seekg(0, std::ios::end);
	long long size = input.tellg();
	input.seekg(0, std::ios::beg);
	/*约定传输的第一个数字代表传输信息总长度*/
	IOData* sendData = getNewBlock(RW_MODE::WRITE);
	sprintf(sendData->buffer, "%lld ", size);
	sendData->wsaBuf.len = strlen(sendData->buffer);
	WSASend(cHandle.clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
	/*将数据划分为BUF_SIZE大小的块传输*/
	for (int i = 0; i < int(ceil(size / (BUF_SIZE * 1.0))); ++i)
	{
		IOData* sendData = getNewBlock(RW_MODE::WRITE);
		input.read(sendData->buffer, min(BUF_SIZE, size - i * BUF_SIZE));
		sendData->wsaBuf.len = min(BUF_SIZE, size - i * BUF_SIZE);
		WSASend(cHandle.clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
	}
	input.close();
}
/**
* @brief	封装读取并保存文件的逻辑
* @param	fName			文件名
* @param	ClntHandleData	客户端句柄对象
*
*/
void FTPServer::readFile(std::string fName, const ClntHandleData& cHandle)
{
	// 写方式打开文件
	std::ofstream file(fName, std::ios::binary);
	IOData* recvData = getNewBlock(RW_MODE::READ);
	bool firstPack = true;
	long long totalLen = 0;
	int pos = 0;
	/*写文件直到末尾，每次读取BUF_SIZE-1大小*/
	while (true && file)
	{
		recvData->wsaBuf.len = BUF_SIZE;
		int recvBytes = recv(cHandle.clntSock, (char*)&recvData->buffer, recvData->wsaBuf.len - 1, 0);
		recvData->buffer[recvBytes] = '\0';
		if (firstPack)	/*从第一个包中读取出文件总长度*/
		{
			// 获取数据总长度
			while (pos < recvBytes && recvData->buffer[pos] != ' ') pos++;
			pos++;
			totalLen = atoll(recvData->buffer);
			firstPack = false;
		}
		// 写入文件
		file.write(recvData->buffer + pos, recvBytes - pos);
		totalLen -= recvBytes - pos;
		if (!totalLen) break;
		pos = 0;
	}
	if (!file)
	{
		throw std::exception("创建文件失败");
	}
	file.close();
}
/**
* @brief	封装传输文件列表的逻辑
* @param	list			文件信息列表
* @param	ClntHandleData	客户端句柄对象
*
*/
void FTPServer::transList(const std::vector<_finddata_t>& list, const ClntHandleData& cHandle)
{
	/*构造格式化字符串流*/
	std::ostringstream oss;
	/*左对齐，不足补空格*/
	oss << std::setw(30) << std::setfill(' ') << std::left << "name"
		<< std::setw(30) << std::setfill(' ') << std::left << "size"
		<< std::setw(30) << std::setfill(' ') << std::left << "modified time" << std::endl;
	for (const auto& info : list)
	{
		/*格式化时间输出*/
		std::tm* tm = std::localtime(&info.time_access);
		std::ostringstream oss_t;
		oss_t << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
		oss << std::setw(30) << std::setfill(' ') << std::left << info.name
			<< std::setw(30) << std::setfill(' ') << std::left << std::to_string(info.size) + " B"
			<< std::setw(30) << std::setfill(' ') << std::left << oss_t.str() << std::endl;
	}
	std::string msg = oss.str();
	/*构造数据包，划分为BUF_SIZE的块传输*/
	msg = std::to_string(msg.length())+ " " + msg;
	for (int i = 0; i < int(ceil(msg.length() / (BUF_SIZE * 1.0))); ++i)
	{
		IOData* sendData = getNewBlock(RW_MODE::WRITE);
		sprintf(sendData->buffer, msg.substr(i * BUF_SIZE, min(BUF_SIZE, msg.length() - i * BUF_SIZE)).c_str());
		sendData->wsaBuf.len = strlen(sendData->buffer);
		WSASend(cHandle.clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
	}
}
/**
* @brief	封装传输字符串消息的逻辑
* @param	ClntHandleData*	客户端句柄对象指针
* @param	string			消息
*
*/
void FTPServer::transPrompt(ClntHandleData* cHandle, std::string msg)
{
	IOData* sendData = getNewBlock(RW_MODE::WRITE);
	sprintf(sendData->buffer, msg.c_str());
	sendData->wsaBuf.len = strlen(sendData->buffer);
	WSASend(cHandle->clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
}
/**
* @brief	新建一个IO数据对象
* @param	RW_MODE		读写模式
*
*/
IOData* FTPServer::getNewBlock(RW_MODE mode)
{
	IOData* res = (IOData*)malloc(sizeof(IOData));
	memset(&(res->overlapped), 0, sizeof(OVERLAPPED));
	res->wsaBuf.buf = res->buffer;
	res->wsaBuf.len = BUF_SIZE;
	res->rwMode = mode;
	return res;
}
/**
* @brief	处理GET指令
* @param	ClntHandleData*	客户端句柄对象指针
* @param	IOData*			IO数据对象指针
* @param	filePath		文件路径
*
*/
void FTPServer::handleGet(ClntHandleData* cHandle, IOData* ioData, std::string fileName)
{
	try
	{
		if (!isFileAvailable(fileName))
		{
			transPrompt(cHandle, std::string("文件不可达"));
		}
		else
		{
			transFile(fileName, *cHandle);
		}
	}
	catch (std::exception e)
	{
		transPrompt(cHandle, std::string(e.what()));
	}
	/*接收下一次用户消息*/
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandle->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}
/**
* @brief	处理PUT指令
* @param	ClntHandleData*	客户端句柄对象指针
* @param	IOData*			IO数据对象指针
* @param	filePath		文件路径
*
*/
void FTPServer::handlePut(ClntHandleData* cHandle, IOData* ioData, std::string fileName)
{
	try
	{
		std::filesystem::path fName(fileName);
		readFile(fileName, *cHandle);
	}
	catch (std::exception e)
	{
		puts(e.what());
	}
	/*接收下一次用户消息*/
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandle->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}

