#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <io.h>
#include <filesystem>
#include <fstream>
#include "FTPServer.h"


const ULONG_PTR FTPServer::COMPLETION_KEY_SHUTDOWN = (ULONG_PTR)-1;


FTPServer::FTPServer(std::string pDic, u_short pPort) : dic(pDic)
{
	WSADATA wsaData;
	SYSTEM_INFO sysInfo;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("初始化WSA失败");
	}

	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hComPort == NULL)
	{
		throw std::runtime_error("创建完成端口失败");
	}
	GetSystemInfo(&sysInfo);
	HANDLE wtHandle;
	for (int i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
	{
		wtHandle = (HANDLE)_beginthreadex(NULL, 0, FTPServer::ftpThreadMain, (LPVOID)this, 0, NULL);
		if (wtHandle == NULL)
		{
			throw std::runtime_error("创建工作线程失败");
		}
		workerHandles.push_back(wtHandle);
	}

	hServerSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hServerSock == INVALID_SOCKET)
	{
		throw std::runtime_error("创建WSA套接字失败:"+WSAGetLastError());
	}
	servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(pPort);

	if (bind(hServerSock, (SOCKADDR*)&servAddr, sizeof(servAddr)))
	{
		throw std::runtime_error("绑定套接字失败" + WSAGetLastError());
	}
	listen(hServerSock, 5);
	int recvBytes, flags = 0;

	while (true)
	{
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);
		SOCKET hClntSock = accept(hServerSock, (SOCKADDR*)&clntAddr, &addrLen);
		clntSocks.insert(hClntSock);
		ClntHandleData* handleInfo = (ClntHandleData*)malloc(sizeof(ClntHandleData));
		handleInfo->clntSock = hClntSock;
		memcpy(&(handleInfo->clntAddr), &clntAddr, addrLen);

		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		IOData* ioInfo = (IOData*)malloc(sizeof(IOData));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = RW_MODE::READ;
		WSARecv(handleInfo->clntSock, &(ioInfo->wsaBuf), 1, (LPDWORD)&recvBytes, (LPDWORD)&flags, &(ioInfo->overlapped), NULL);
	}
}

FTPServer::~FTPServer()
{
	// 向每个线程发送一个关闭信号。在这个例子中，关闭信号是一个特殊的完成键。
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
	for (auto sock : clntSocks)
	{
		closesocket(sock);
	}
	closesocket(hServerSock);
	WSACleanup();
}

void FTPServer::handleList(ClntHandleData* cHandleData, IOData* ioData, std::string dic)
{
	auto fileNameList = getFileNameList(dic);
	//puts(dic.c_str());
	transList(fileNameList, *cHandleData);

	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandleData->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}

void FTPServer::handleQuit(ClntHandleData* cHandleData, IOData* ioData, std::unordered_set<SOCKET>&clntSocks)
{
	free(ioData);
	clntSocks.erase(cHandleData->clntSock);
	closesocket(cHandleData->clntSock);
}

void FTPServer::handleUnknown(ClntHandleData* cHandleData, IOData* ioData)
{
	IOData* sendData = getNewBlock(RW_MODE::WRITE);
	sprintf(sendData->buffer, "ERROR: Unknown Command.");
	sendData->wsaBuf.len = strlen(sendData->wsaBuf.buf);
	WSASend(cHandleData->clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);

	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandleData->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}

std::vector<std::string> FTPServer::getFileNameList(std::string dic)
{
	std::vector<std::string> res;
	long hFile = 0;
	struct _finddata_t fileinfo;
	std::string p;
	if ((hFile = _findfirst(p.assign(dic).append("*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//std::cout << fileinfo.name;
			if (!(fileinfo.attrib & _A_SUBDIR))
			{
				res.push_back(fileinfo.name);
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	} 
	return res;
}

bool FTPServer::isFileAvailable(std::string filename)
{
	std::filesystem::path fName(filename);
	if (!std::filesystem::exists(fName))
	{
		throw std::exception("文件不存在");
	}
	if (std::filesystem::is_directory(fName))
	{
		throw std::exception("不支持传输文件夹");
	}
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

std::pair<FTP_COMMAND, std::string> FTPServer::readCommand(const IOData* ioData)
{
	std::stringstream is(ioData->buffer);
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

DWORD32 WINAPI FTPServer::ftpThreadMain(LPVOID serverPtr_)
{
	HANDLE hComPort = ((FTPServer*)serverPtr_)->hComPort;
	IOData* ioInfo;
	ClntHandleData* handleInfo;
	DWORD bytesTrans;
	DWORD flags = 0;
	
	while (true)
	{
		boolean result = GetQueuedCompletionStatus(hComPort, &bytesTrans, (PULONG_PTR)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		if (result)
		{
			if ((ULONG_PTR)handleInfo == FTPServer::COMPLETION_KEY_SHUTDOWN)
			{
				break;
			}
			SOCKET clntSock = handleInfo->clntSock;
			if (ioInfo->rwMode == RW_MODE::READ)
			{
				if (bytesTrans == 0)
				{
					continue;
				}
				ioInfo->buffer[ioInfo->wsaBuf.len] = '\0';
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
			else if (ioInfo->rwMode == RW_MODE::WRITE)
			{
				free(ioInfo);
			}

		}
	}
	return DWORD32();
}

void FTPServer::transFile(std::string fName, const ClntHandleData& cHandle)
{
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
	IOData* sendData = getNewBlock(RW_MODE::WRITE);
	sprintf(sendData->buffer, "%lld ", size);
	sendData->wsaBuf.len = strlen(sendData->buffer);
	//puts(sendData->wsaBuf.buf);
	WSASend(cHandle.clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);

	for (int i = 0; i < int(ceil(size / (BUF_SIZE * 1.0))); ++i)
	{
		IOData* sendData = getNewBlock(RW_MODE::WRITE);
		input.read(sendData->buffer, min(BUF_SIZE, size - i * BUF_SIZE));
		sendData->wsaBuf.len = min(BUF_SIZE, size - i * BUF_SIZE);
		WSASend(cHandle.clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
	}
	input.close();
}

void FTPServer::readFile(std::string fName, const ClntHandleData& cHandle)
{
	std::ofstream file(fName);
	IOData* recvData = getNewBlock(RW_MODE::READ);
	bool firstPack = true;
	long long totalLen = 0;
	int pos = 0;
	while (true && file)
	{
		recvData->wsaBuf.len = BUF_SIZE;
		int recvBytes = recv(cHandle.clntSock, (char*)&recvData->buffer, recvData->wsaBuf.len - 1, 0);
		recvData->buffer[recvBytes] = '\0';
		if (firstPack)
		{
			while (pos < recvBytes && recvData->buffer[pos] != ' ') pos++;
			pos++;
			totalLen = atoll(recvData->buffer);
			firstPack = false;
		}
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

void FTPServer::transList(const std::vector<std::string>& list, const ClntHandleData& cHandle)
{
	std::string msg = "";
	int cnt = 0;
	for (auto name : list)
	{

		msg.append(name);
		msg.append((cnt % 5 || cnt == 0) ? " " : "\n");
		cnt++;
	}
	msg = std::to_string(msg.length())+ " " + msg;
	for (int i = 0; i < int(ceil(msg.length() / (BUF_SIZE * 1.0))); ++i)
	{
		IOData* sendData = getNewBlock(RW_MODE::WRITE);
		sprintf(sendData->buffer, msg.substr(i * BUF_SIZE, min(BUF_SIZE, msg.length() - i * BUF_SIZE)).c_str());
		sendData->wsaBuf.len = strlen(sendData->buffer);
		WSASend(cHandle.clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
	}
}

void FTPServer::transPrompt(ClntHandleData* cHandle, std::string msg)
{
	IOData* sendData = getNewBlock(RW_MODE::WRITE);
	sprintf(sendData->buffer, msg.c_str());
	sendData->wsaBuf.len = strlen(sendData->buffer);
	WSASend(cHandle->clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
}

IOData* FTPServer::getNewBlock(RW_MODE mode)
{
	IOData* res = (IOData*)malloc(sizeof(IOData));
	memset(&(res->overlapped), 0, sizeof(OVERLAPPED));
	res->wsaBuf.buf = res->buffer;
	res->wsaBuf.len = BUF_SIZE;
	res->rwMode = mode;
	return res;
}

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
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandle->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}

void FTPServer::handlePut(ClntHandleData* cHandle, IOData* ioData, std::string fileName)
{
	try
	{
		std::filesystem::path fName(fileName);
		if (std::filesystem::exists(fName))
		{
			transPrompt(cHandle, std::string("文件已存在"));
		}
		else
		{
			readFile(fileName, *cHandle);
		}
	}
	catch (std::exception e)
	{
		transPrompt(cHandle, std::string(e.what()));
	}
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandle->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}

