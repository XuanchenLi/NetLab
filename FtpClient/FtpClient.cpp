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


FTPClient::FTPClient(std::string dir, std::string servIP, u_short servPort)
{

	buffer = new char[BUF_SIZE];

	this->dic = dir;
	memset(&servAddr, 0, sizeof(SOCKADDR_IN));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(servIP.c_str());
	servAddr.sin_port = htons(servPort);
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::exception("初始化WSA失败");
	}

	clntSock = socket(PF_INET, SOCK_STREAM, 0);
	if (clntSock == INVALID_SOCKET)
	{
		throw std::exception("初始化套接字失败");
	}
	int flag = 1;
	int result = setsockopt(clntSock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
	if (result == SOCKET_ERROR)
	{
		throw std::exception("关闭Nagle算法失败");
	}

	if (connect(clntSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == -1)
	{
		throw std::exception("连接服务器失败");
	}

	while (true)
	{
		printf("FTPClient>");
		fgets(buffer, BUF_SIZE, stdin);

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
			break;
		case::FTP_COMMAND::UNKNOWN:
			handleUnknown();
			break;
		}

	}
}


FTPClient::~FTPClient()
{
	delete[] buffer;
	closesocket(clntSock);
	WSACleanup();
}

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

	for (int i = 0; i < int(ceil(size / (BUF_SIZE * 1.0))); ++i)
	{
		input.read(buffer, min(BUF_SIZE, size - i * BUF_SIZE));
		send(clntSock, buffer, min(BUF_SIZE, size - i * BUF_SIZE), 0);
	}
	input.close();
}

void FTPClient::readFile(std::string fName)
{
	std::ofstream file(fName);
	bool firstPack = true;
	long long totalLen = 0;
	int pos = 0;
	while (true && file)
	{
		int recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
		buffer[recvBytes] = '\0';
		if (firstPack)
		{
			while (pos < recvBytes && buffer[pos] != ' ') pos++;
			pos++;
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

void FTPClient::handleUnknown()
{
	send(clntSock, buffer, strlen(buffer) + 1, 0);
	int recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
	buffer[recvBytes] = '\0';
	puts(buffer);
}

void FTPClient::handleQuit()
{
	send(clntSock, buffer, strlen(buffer) + 1, 0);
	puts("Bye~");
}

void FTPClient::handleList()
{
	send(clntSock, buffer, strlen(buffer) + 1, 0);
	//puts(buffer);
	int recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
	buffer[recvBytes] = '\0';
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
	}
	puts(msg.c_str());
}

void FTPClient::handlePut(std::string fName)
{
	try
	{
		if (!isFileAvailable(fName))
		{
			throw std::exception("文件不可达");
		}
		send(clntSock, buffer, strlen(buffer) + 1, 0);
		transFile(fName);
	}
	catch (std::exception e)
	{
		puts(e.what());
		return;
	}
}

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
