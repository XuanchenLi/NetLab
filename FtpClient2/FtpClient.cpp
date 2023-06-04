/*
* @File: FTPClient.h
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/6/1 下午 4:32
* @Description: 定义了FTP客户端的各种方法
* Version V1.0
*/
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
#include "FTPClient.h"

/**
* @brief	客户端对象构造函数
* @param	dir	当前工作目录
* @param	servIP	服务器IP
* @param	servPort	服务器端口
*/
FTPClient::FTPClient(std::string dir, std::string servIP, u_short servPort)
{

	buffer = new char[BUF_SIZE];
	/*初始化服务器地址*/
	this->dic = dir;
	memset(&servAddr, 0, sizeof(SOCKADDR_IN));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(servIP.c_str());
	servAddr.sin_port = htons(servPort);
	WSADATA wsaData;
	/*初始化WSA环境*/
	puts("初始化WSA...");
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::exception("初始化WSA失败");
	}
	/*创建客户端套接字*/
	puts("创建套接字...");
	clntSock = socket(PF_INET, SOCK_STREAM, 0);
	if (clntSock == INVALID_SOCKET)
	{
		throw std::exception("初始化套接字失败");
	}
	/*关闭Nagle算法*/
	int flag = 1;
	int result = setsockopt(clntSock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
	if (result == SOCKET_ERROR)
	{
		throw std::exception("关闭Nagle算法失败");
	}
	/*连接服务器*/
	puts("连接服务器...");
	if (connect(clntSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == -1)
	{
		throw std::exception("连接服务器失败");
	}
	puts("连接FTP服务器成功");
	/*循环处理用户命令*/
	while (true)
	{
		printf("FTPClient>");
		fgets(buffer, BUF_SIZE, stdin);
		/*根据输入指令转到相应处理函数*/
		auto cmd = readCommand();
		switch (cmd.first)
		{
		case::FTP_COMMAND::GET:
			handleGet(dir + cmd.second);
			break;
		case::FTP_COMMAND::PUT:
			handlePut(dir + cmd.second);
			break;
		case::FTP_COMMAND::LIST:
			handleList();
			break;
		case::FTP_COMMAND::QUIT:
			handleQuit();
			return;
			break;
		case::FTP_COMMAND::UNKNOWN:
			handleUnknown();
			break;
		}

	}
}

/**
* @brief	客户端对象析构函数，释放所有资源
*/
FTPClient::~FTPClient()
{
	delete[] buffer;
	closesocket(clntSock);
	WSACleanup();
}
/**
* @brief	从缓冲区中读取用户发送的指令和参数
*
* @return	std::pair<FTP_COMMAND, std::string>	指令和参数
*/
std::pair<FTP_COMMAND, std::string> FTPClient::readCommand()
{
	std::stringstream is(buffer);
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
* @brief	封装传输文件的逻辑
* @param	fName			文件名
*
*/
void FTPClient::transFile(std::string fName)
{
	std::ifstream input(fName.c_str(), std::ios::binary);
	// 检查文件是否正确打开
	if (!input)
	{
		sprintf(buffer, "0 ");
		send(clntSock, buffer, strlen(buffer), 0);
		throw std::exception("打开文件失败");
	}
	// 获取文件大小
	input.seekg(0, std::ios::end);
	long long size = input.tellg();
	input.seekg(0, std::ios::beg);
	sprintf(buffer, "%lld ", size);
	send(clntSock, buffer, strlen(buffer), 0);
	// 将文件划分为BUF_SIZE大小数据包传输
	for (int i = 0; i < int(ceil(size / (BUF_SIZE * 1.0))); ++i)
	{
		input.read(buffer, min(BUF_SIZE, size - i * BUF_SIZE));
		send(clntSock, buffer, min(BUF_SIZE, size - i * BUF_SIZE), 0);
	}
	input.close();
}
/**
* @brief	封装读取并保存文件的逻辑
* @param	fName			文件名
*
*/
void FTPClient::readFile(std::string fName)
{
	/*打开文件*/
	std::ofstream file(fName, std::ios::binary);
	bool firstPack = true;
	long long totalLen = 0;
	int pos = 0;
	/*不断读取数据直到结束*/
	while (true && file)
	{
		int recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
		buffer[recvBytes] = '\0';
		if (firstPack)	// 从第一个数据包中读出数据总长度
		{
			while (pos < recvBytes && buffer[pos] != ' ') pos++;
			pos++;
			if (!(buffer[0] >= '0' && buffer[0] <= '9'))
			{
				puts(buffer);
				file.close();
				remove(fName.c_str());
				return;
			}
			totalLen = atoll(buffer);
			firstPack = false;
		}
		file.write(buffer + pos, recvBytes - pos);
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
* @brief	判断文件是否可用
* @param	string						文件路径
*
* @return	bool						True代表可用
*/
bool FTPClient::isFileAvailable(std::string filename)
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
		if ((std::filesystem::status(fName).permissions() & std::filesystem::perms::owner_read) != std::filesystem::perms::none)
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
* @brief	处理未识别的指令
*
* @return	std::vector<_finddata_t>	文件信息列表
*/
void FTPClient::handleUnknown()
{
	send(clntSock, buffer, strlen(buffer) + 1, 0);
	int recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
	buffer[recvBytes] = '\0';
	puts(buffer);
}
/**
* @brief	处理QUIT指令
*
*/
void FTPClient::handleQuit()
{
	send(clntSock, buffer, strlen(buffer) + 1, 0);
	puts("Bye~");
}
/**
* @brief	处理LIST指令
*
*/
void FTPClient::handleList()
{
	send(clntSock, buffer, strlen(buffer) + 1, 0);
	/*接收列表消息*/
	int recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
	buffer[recvBytes] = '\0';
	/*读出列表总长度*/
	long long totLen = atoll(buffer);
	int pos = 0;
	while (buffer[pos] != ' ') pos++;
	pos++;
	std::string msg(buffer + pos);
	totLen -= recvBytes - pos;
	while (totLen)
	{
		recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
		buffer[recvBytes] = '\0';
		msg += buffer;
		totLen -= recvBytes;
	}
	puts(msg.c_str());
}
/**
* @brief	处理PUT指令
* @param	filePath		文件路径
*
*/
void FTPClient::handlePut(std::string fName)
{
	try
	{
		if (!isFileAvailable(fName))
		{
			throw std::exception("文件不可达");
		}
		/*传输指令和文件*/
		send(clntSock, buffer, strlen(buffer) + 1, 0);
		transFile(fName);
	}
	catch (std::exception e)
	{
		puts(e.what());
		return;
	}
}
/**
* @brief	处理GET指令
* @param	filePath		文件路径
*
*/
void FTPClient::handleGet(std::string fileName)
{
	try
	{
		std::filesystem::path fName(fileName);
		if (std::filesystem::exists(fName))
		{
			puts("文件已存在");
		}
		else
		{
			/*接收文件*/
			send(clntSock, buffer, strlen(buffer) + 1, 0);
			readFile(fileName);
		}
	}
	catch (std::exception e)
	{
		puts(e.what());
		return;
	}
}
