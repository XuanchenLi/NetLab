/*
* @File: FTPClient.h
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/6/1 ���� 4:32
* @Description: ������FTP�ͻ��˵ĸ��ַ���
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
* @brief	�ͻ��˶����캯��
* @param	dir	��ǰ����Ŀ¼
* @param	servIP	������IP
* @param	servPort	�������˿�
*/
FTPClient::FTPClient(std::string dir, std::string servIP, u_short servPort)
{

	buffer = new char[BUF_SIZE];
	/*��ʼ����������ַ*/
	this->dic = dir;
	memset(&servAddr, 0, sizeof(SOCKADDR_IN));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(servIP.c_str());
	servAddr.sin_port = htons(servPort);
	WSADATA wsaData;
	/*��ʼ��WSA����*/
	puts("��ʼ��WSA...");
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::exception("��ʼ��WSAʧ��");
	}
	/*�����ͻ����׽���*/
	puts("�����׽���...");
	clntSock = socket(PF_INET, SOCK_STREAM, 0);
	if (clntSock == INVALID_SOCKET)
	{
		throw std::exception("��ʼ���׽���ʧ��");
	}
	/*�ر�Nagle�㷨*/
	int flag = 1;
	int result = setsockopt(clntSock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
	if (result == SOCKET_ERROR)
	{
		throw std::exception("�ر�Nagle�㷨ʧ��");
	}
	/*���ӷ�����*/
	puts("���ӷ�����...");
	if (connect(clntSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == -1)
	{
		throw std::exception("���ӷ�����ʧ��");
	}
	puts("����FTP�������ɹ�");
	/*ѭ�������û�����*/
	while (true)
	{
		printf("FTPClient>");
		fgets(buffer, BUF_SIZE, stdin);
		/*��������ָ��ת����Ӧ������*/
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
* @brief	�ͻ��˶��������������ͷ�������Դ
*/
FTPClient::~FTPClient()
{
	delete[] buffer;
	closesocket(clntSock);
	WSACleanup();
}
/**
* @brief	�ӻ������ж�ȡ�û����͵�ָ��Ͳ���
*
* @return	std::pair<FTP_COMMAND, std::string>	ָ��Ͳ���
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
* @brief	��װ�����ļ����߼�
* @param	fName			�ļ���
*
*/
void FTPClient::transFile(std::string fName)
{
	std::ifstream input(fName.c_str(), std::ios::binary);
	// ����ļ��Ƿ���ȷ��
	if (!input)
	{
		sprintf(buffer, "0 ");
		send(clntSock, buffer, strlen(buffer), 0);
		throw std::exception("���ļ�ʧ��");
	}
	// ��ȡ�ļ���С
	input.seekg(0, std::ios::end);
	long long size = input.tellg();
	input.seekg(0, std::ios::beg);
	sprintf(buffer, "%lld ", size);
	send(clntSock, buffer, strlen(buffer), 0);
	// ���ļ�����ΪBUF_SIZE��С���ݰ�����
	for (int i = 0; i < int(ceil(size / (BUF_SIZE * 1.0))); ++i)
	{
		input.read(buffer, min(BUF_SIZE, size - i * BUF_SIZE));
		send(clntSock, buffer, min(BUF_SIZE, size - i * BUF_SIZE), 0);
	}
	input.close();
}
/**
* @brief	��װ��ȡ�������ļ����߼�
* @param	fName			�ļ���
*
*/
void FTPClient::readFile(std::string fName)
{
	/*���ļ�*/
	std::ofstream file(fName, std::ios::binary);
	bool firstPack = true;
	long long totalLen = 0;
	int pos = 0;
	/*���϶�ȡ����ֱ������*/
	while (true && file)
	{
		int recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
		buffer[recvBytes] = '\0';
		if (firstPack)	// �ӵ�һ�����ݰ��ж��������ܳ���
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
		throw std::exception("�����ļ�ʧ��");
	}
	file.close();
}
/**
* @brief	�ж��ļ��Ƿ����
* @param	string						�ļ�·��
*
* @return	bool						True�������
*/
bool FTPClient::isFileAvailable(std::string filename)
{
	std::filesystem::path fName(filename);
	if (!std::filesystem::exists(fName))
	{
		throw std::exception("�ļ�������");
	}
	if (std::filesystem::is_directory(fName))
	{
		throw std::exception("��֧�ִ����ļ���");
	}
	if (std::filesystem::is_regular_file(fName) && std::filesystem::status_known(std::filesystem::status(fName)))
	{
		if ((std::filesystem::status(fName).permissions() & std::filesystem::perms::owner_read) != std::filesystem::perms::none)
		{
			return true;
		}
		else
		{
			throw std::exception("�ļ����ɶ�");
		}
	}
	else
	{
		throw std::exception("��֧�ֵ��ļ�����");
	}
	return false;
}
/**
* @brief	����δʶ���ָ��
*
* @return	std::vector<_finddata_t>	�ļ���Ϣ�б�
*/
void FTPClient::handleUnknown()
{
	send(clntSock, buffer, strlen(buffer) + 1, 0);
	int recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
	buffer[recvBytes] = '\0';
	puts(buffer);
}
/**
* @brief	����QUITָ��
*
*/
void FTPClient::handleQuit()
{
	send(clntSock, buffer, strlen(buffer) + 1, 0);
	puts("Bye~");
}
/**
* @brief	����LISTָ��
*
*/
void FTPClient::handleList()
{
	send(clntSock, buffer, strlen(buffer) + 1, 0);
	/*�����б���Ϣ*/
	int recvBytes = recv(clntSock, buffer, BUF_SIZE - 1, 0);
	buffer[recvBytes] = '\0';
	/*�����б��ܳ���*/
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
* @brief	����PUTָ��
* @param	filePath		�ļ�·��
*
*/
void FTPClient::handlePut(std::string fName)
{
	try
	{
		if (!isFileAvailable(fName))
		{
			throw std::exception("�ļ����ɴ�");
		}
		/*����ָ����ļ�*/
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
* @brief	����GETָ��
* @param	filePath		�ļ�·��
*
*/
void FTPClient::handleGet(std::string fileName)
{
	try
	{
		std::filesystem::path fName(fileName);
		if (std::filesystem::exists(fName))
		{
			puts("�ļ��Ѵ���");
		}
		else
		{
			/*�����ļ�*/
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
