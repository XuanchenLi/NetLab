/*
* @File: FTPServer.cpp
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/29 ���� 5:06
* @Description: ������FTP���������ĸ��ַ���
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
* @brief	������ֹ�̵߳�ָ��߳̽��յ�����Ϣʱ��������
*/
const ULONG_PTR FTPServer::COMPLETION_KEY_SHUTDOWN = (ULONG_PTR)-1;

/**
* @brief	�����������캯��
* @param	pDic	��ǰ����Ŀ¼
* @param	pPort	�������˿�
*/
FTPServer::FTPServer(std::string pDic, u_short pPort) : dic(pDic)
{
	/*��ʼ��WSA*/
	WSADATA wsaData;
	SYSTEM_INFO sysInfo;
	puts("��ʼ��WSA...");
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("��ʼ��WSAʧ��");
	}
	/*������ɶ˿�*/
	puts("������ɶ˿�...");
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hComPort == NULL)
	{
		throw std::runtime_error("������ɶ˿�ʧ��");
	}
	/*���������߳�*/
	GetSystemInfo(&sysInfo);
	HANDLE wtHandle;
	puts("���������߳�...");
	for (int i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
	{
		wtHandle = (HANDLE)_beginthreadex(NULL, 0, FTPServer::ftpThreadMain, (LPVOID)this, 0, NULL);
		if (wtHandle == NULL)
		{
			throw std::runtime_error("���������߳�ʧ��");
		}
		workerHandles.push_back(wtHandle);
	}
	/*�����������׽���*/
	puts("�����׽���...");
	hServerSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hServerSock == INVALID_SOCKET)
	{
		throw std::runtime_error("����WSA�׽���ʧ��:" + WSAGetLastError());
	}
	/*���׽���*/
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(pPort);
	puts("���׽���...");
	if (bind(hServerSock, (SOCKADDR*)&servAddr, sizeof(servAddr)))
	{
		throw std::runtime_error("���׽���ʧ��" + WSAGetLastError());
	}
	puts("��ʼ����...");
	listen(hServerSock, 5);
	int recvBytes, flags = 0;
	puts("FTP�����������ɹ�");
	/*������ѭ�����տͻ�������*/
	while (true)
	{
		/*�������ӣ���ʼ���ͻ��˾������*/
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);
		SOCKET hClntSock = accept(hServerSock, (SOCKADDR*)&clntAddr, &addrLen);
		clntSocks.insert(hClntSock);
		ClntHandleData* handleInfo = (ClntHandleData*)malloc(sizeof(ClntHandleData));
		handleInfo->clntSock = hClntSock;
		memcpy(&(handleInfo->clntAddr), &clntAddr, addrLen);
		printf("�ͻ���[%s:%d]���ӳɹ�\n", inet_ntoa(*((in_addr*)&(handleInfo->clntAddr.sin_addr))), ntohs(handleInfo->clntAddr.sin_port));
		/*�󶨿ͻ����׽��ֵ���ɶ˿ڶ���*/
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);
		/*����IO���ݶ����û��첽���տͻ�����Ϣ*/
		IOData* ioInfo = (IOData*)malloc(sizeof(IOData));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = RW_MODE::READ;
		WSARecv(handleInfo->clntSock, &(ioInfo->wsaBuf), 1, (LPDWORD)&recvBytes, (LPDWORD)&flags, &(ioInfo->overlapped), NULL);
	}
}
/**
* @brief	���������������������ͷ�������Դ
*/
FTPServer::~FTPServer()
{
	// ��ÿ���̷߳���һ���ر��źš��ر��ź���һ���������ɼ���
	for (size_t i = 0; i < workerHandles.size(); ++i)
	{
		PostQueuedCompletionStatus(hComPort, 0, COMPLETION_KEY_SHUTDOWN, NULL);
	}

	// �ȴ����й����̹߳رա�
	WaitForMultipleObjects(static_cast<DWORD>(workerHandles.size()), &workerHandles[0], TRUE, INFINITE);

	// �ر���ɶ˿ڡ�
	CloseHandle(hComPort);

	// �ر��߳̾����
	for (size_t i = 0; i < workerHandles.size(); ++i)
	{
		CloseHandle(workerHandles[i]);
	}
	// �رտͻ����׽���
	for (auto sock : clntSocks)
	{
		closesocket(sock);
	}
	/*�رշ������׽���*/
	closesocket(hServerSock);
	/*���WSA����*/
	WSACleanup();
}
/**
* @brief	����LISTָ��
* @param	ClntHandleData*	�ͻ��˾������ָ��
* @param	IOData*			IO���ݶ���ָ��
* @param	string			Ŀ¼·��
*
*/
void FTPServer::handleList(ClntHandleData* cHandleData, IOData* ioData, std::string dic)
{
	/*��ȡ�ļ���Ϣ*/
	auto fileNameList = getFileInfoList(dic);
	/*�����ļ���Ϣ*/
	transList(fileNameList, *cHandleData);
	/*�첽�����´��û����͵���Ϣ*/
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandleData->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}
/**
* @brief	����QUITָ��
* @param	ClntHandleData*				�ͻ��˾������ָ��
* @param	IOData*						IO���ݶ���ָ��
* @param	std::unordered_set<SOCKET>	FTP������ά���Ŀͻ��˾������
*
*/
void FTPServer::handleQuit(ClntHandleData* cHandleData, IOData* ioData, std::unordered_set<SOCKET>&clntSocks)
{
	/*�ͷ���Դ*/
	free(ioData);
	clntSocks.erase(cHandleData->clntSock);
	closesocket(cHandleData->clntSock);
	printf("�ͻ���[%s:%d]�Ͽ�����\n", inet_ntoa(*((in_addr*)&(cHandleData->clntAddr.sin_addr))), ntohs(cHandleData->clntAddr.sin_port));
}
/**
* @brief	����δʶ���ָ��
* @param	string						Ŀ¼·��
*
* @return	std::vector<_finddata_t>	�ļ���Ϣ�б�
*/
void FTPServer::handleUnknown(ClntHandleData* cHandleData, IOData* ioData)
{
	/*������ʾ��Ϣ*/
	IOData* sendData = getNewBlock(RW_MODE::WRITE);
	sprintf(sendData->buffer, "ERROR: Unknown Command.");
	sendData->wsaBuf.len = strlen(sendData->wsaBuf.buf);
	WSASend(cHandleData->clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
	/*������һ����Ϣ*/
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandleData->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}
/**
* @brief	��ȡ�ļ���Ϣ�б�
* @param	string						Ŀ¼·��
*
* @return	std::vector<_finddata_t>	�ļ���Ϣ�б�
*/
std::vector<_finddata_t> FTPServer::getFileInfoList(std::string dic)
{
	std::vector<_finddata_t> res;
	long hFile = 0;
	struct _finddata_t fileinfo;
	std::string p;
	/*����Ŀ¼�е����з��ļ����ļ�*/
	if ((hFile = _findfirst(p.assign(dic).append("*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			if (!(fileinfo.attrib & _A_SUBDIR))
			{
				res.push_back(fileinfo);
			}
		} while (_findnext(hFile, &fileinfo) == 0);	//����ֵλ0�������¸��ļ�
		_findclose(hFile);	// �رվ��
	} 
	return res;
}
/**
* @brief	�ж��ļ��Ƿ����
* @param	string						�ļ�·��
*
* @return	bool						True�������
*/
bool FTPServer::isFileAvailable(std::string filename)
{
	std::filesystem::path fName(filename);
	/*�ж��ļ�����*/
	if (!std::filesystem::exists(fName))
	{
		throw std::exception("�ļ�������");
	}
	/*�ж��Ƿ�ΪĿ¼*/
	if (std::filesystem::is_directory(fName))
	{
		throw std::exception("��֧�ִ����ļ���");
	}
	/*�ж��ļ�״̬��Ȩ����Ϣ*/
	if (std::filesystem::is_regular_file(fName) && std::filesystem::status_known(std::filesystem::status(fName)))
	{
		if ((std::filesystem::status(fName).permissions() & std::filesystem::perms::others_read) != std::filesystem::perms::none)
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
* @brief	�ӻ������ж�ȡ�û����͵�ָ��Ͳ���
* @param	IOData		IO���ݶ���
*
* @return	std::pair<FTP_COMMAND, std::string>	ָ��Ͳ���
*/
std::pair<FTP_COMMAND, std::string> FTPServer::readCommand(const IOData* ioData)
{
	std::stringstream is(ioData->buffer);	//�ַ���������
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
* @brief	IOCP�����߳�������
* @param	serverPtr_		����������ָ��
*
* @return	DWORD32
*/
DWORD32 WINAPI FTPServer::ftpThreadMain(LPVOID serverPtr_)
{
	/*ͨ�������������ȡ��ɶ˿�*/
	HANDLE hComPort = ((FTPServer*)serverPtr_)->hComPort;
	IOData* ioInfo;
	ClntHandleData* handleInfo;
	DWORD bytesTrans;
	DWORD flags = 0;

	/*�����߳�ѭ������IO�����Ϣ*/
	while (true)
	{
		/*�ȴ�����ȡ����ɵ�IO��Ϣ�����û��������*/
		boolean result = GetQueuedCompletionStatus(hComPort, &bytesTrans, (PULONG_PTR)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		if (result)
		{
			/*�������ֹKEY����ֹ����*/
			if ((ULONG_PTR)handleInfo == FTPServer::COMPLETION_KEY_SHUTDOWN)
			{
				break;
			}
			SOCKET clntSock = handleInfo->clntSock;
			if (ioInfo->rwMode == RW_MODE::READ)	/*������ɵĶ�����*/
			{

				if (bytesTrans == 0)
				{
					continue;
				}
				ioInfo->buffer[ioInfo->wsaBuf.len] = '\0';
				/*�ж��������ͣ�ת����Ӧ������*/
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
			else if (ioInfo->rwMode == RW_MODE::WRITE)	/*������ɵ�д����*/
			{
				// �ͷ�IO������Դ
				free(ioInfo);
			}

		}
	}
	return DWORD32();
}
/**
	* @brief	��װ�����ļ����߼�
	* @param	fName			�ļ���
	* @param	ClntHandleData	�ͻ��˾������
	*
	*/
void FTPServer::transFile(std::string fName, const ClntHandleData& cHandle)
{
	// ����ʽ���ļ�
	std::ifstream input(fName.c_str(), std::ios::binary);
	// ����ļ��Ƿ���ȷ��
	if (!input)
	{
		throw std::exception("���ļ�ʧ��");
	}
	// ��ȡ�ļ���С
	input.seekg(0, std::ios::end);
	long long size = input.tellg();
	input.seekg(0, std::ios::beg);
	/*Լ������ĵ�һ�����ִ�������Ϣ�ܳ���*/
	IOData* sendData = getNewBlock(RW_MODE::WRITE);
	sprintf(sendData->buffer, "%lld ", size);
	sendData->wsaBuf.len = strlen(sendData->buffer);
	WSASend(cHandle.clntSock, &(sendData->wsaBuf), 1, NULL, 0, &(sendData->overlapped), NULL);
	/*�����ݻ���ΪBUF_SIZE��С�Ŀ鴫��*/
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
* @brief	��װ��ȡ�������ļ����߼�
* @param	fName			�ļ���
* @param	ClntHandleData	�ͻ��˾������
*
*/
void FTPServer::readFile(std::string fName, const ClntHandleData& cHandle)
{
	// д��ʽ���ļ�
	std::ofstream file(fName, std::ios::binary);
	IOData* recvData = getNewBlock(RW_MODE::READ);
	bool firstPack = true;
	long long totalLen = 0;
	int pos = 0;
	/*д�ļ�ֱ��ĩβ��ÿ�ζ�ȡBUF_SIZE-1��С*/
	while (true && file)
	{
		recvData->wsaBuf.len = BUF_SIZE;
		int recvBytes = recv(cHandle.clntSock, (char*)&recvData->buffer, recvData->wsaBuf.len - 1, 0);
		recvData->buffer[recvBytes] = '\0';
		if (firstPack)	/*�ӵ�һ�����ж�ȡ���ļ��ܳ���*/
		{
			// ��ȡ�����ܳ���
			while (pos < recvBytes && recvData->buffer[pos] != ' ') pos++;
			pos++;
			totalLen = atoll(recvData->buffer);
			firstPack = false;
		}
		// д���ļ�
		file.write(recvData->buffer + pos, recvBytes - pos);
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
* @brief	��װ�����ļ��б���߼�
* @param	list			�ļ���Ϣ�б�
* @param	ClntHandleData	�ͻ��˾������
*
*/
void FTPServer::transList(const std::vector<_finddata_t>& list, const ClntHandleData& cHandle)
{
	/*�����ʽ���ַ�����*/
	std::ostringstream oss;
	/*����룬���㲹�ո�*/
	oss << std::setw(30) << std::setfill(' ') << std::left << "name"
		<< std::setw(30) << std::setfill(' ') << std::left << "size"
		<< std::setw(30) << std::setfill(' ') << std::left << "modified time" << std::endl;
	for (const auto& info : list)
	{
		/*��ʽ��ʱ�����*/
		std::tm* tm = std::localtime(&info.time_access);
		std::ostringstream oss_t;
		oss_t << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
		oss << std::setw(30) << std::setfill(' ') << std::left << info.name
			<< std::setw(30) << std::setfill(' ') << std::left << std::to_string(info.size) + " B"
			<< std::setw(30) << std::setfill(' ') << std::left << oss_t.str() << std::endl;
	}
	std::string msg = oss.str();
	/*�������ݰ�������ΪBUF_SIZE�Ŀ鴫��*/
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
* @brief	��װ�����ַ�����Ϣ���߼�
* @param	ClntHandleData*	�ͻ��˾������ָ��
* @param	string			��Ϣ
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
* @brief	�½�һ��IO���ݶ���
* @param	RW_MODE		��дģʽ
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
* @brief	����GETָ��
* @param	ClntHandleData*	�ͻ��˾������ָ��
* @param	IOData*			IO���ݶ���ָ��
* @param	filePath		�ļ�·��
*
*/
void FTPServer::handleGet(ClntHandleData* cHandle, IOData* ioData, std::string fileName)
{
	try
	{
		if (!isFileAvailable(fileName))
		{
			transPrompt(cHandle, std::string("�ļ����ɴ�"));
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
	/*������һ���û���Ϣ*/
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandle->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}
/**
* @brief	����PUTָ��
* @param	ClntHandleData*	�ͻ��˾������ָ��
* @param	IOData*			IO���ݶ���ָ��
* @param	filePath		�ļ�·��
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
	/*������һ���û���Ϣ*/
	DWORD flags = 0;
	memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
	ioData->wsaBuf.len = BUF_SIZE;
	ioData->rwMode = RW_MODE::READ;
	WSARecv(cHandle->clntSock, &(ioData->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioData->overlapped), NULL);
}

