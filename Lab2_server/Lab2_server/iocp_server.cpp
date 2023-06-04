/*
* @File: iocp_server.cpp
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/27 ���� 7:00
* @Description: ������IOCP�������������ĸ��ֽṹ�������߼���
*				���������Ͻ��տͻ�����Ϣ�����䷵�ء�
* Version V1.0
*/
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>


#pragma comment(lib, "Ws2_32.lib")

#define BUF_SIZE 1024

/*
* @brief	�����첽�����첽д����
*/
enum class RW_MODE
{
	READ,
	WRITE
};

/**
* @brief	��װ�ͻ��˾����Ϣ
*/
typedef struct
{
	SOCKET clntSock;		/*�ͻ����׽���*/
	SOCKADDR_IN clntAddr;	/*�ͻ��˵�ַ*/
} ClntHandleData;

/**
* @brief	��װIO������
*/
typedef struct
{
	OVERLAPPED overlapped;		/*�ص�IO�������*/
	WSABUF wsaBuf;				/*�ص�IO����������*/
	char buffer[BUF_SIZE];		/*������*/
	RW_MODE rwMode;				/*��дģʽ*/
} IOData;

/**
* @brief	IOCP�����߳�������
* @param	comPort		�ص��˿ڶ���
* 
* @return	DWORD32
*/
DWORD32 WINAPI EchoThreadMain(LPVOID comPort);


/**
* @brief ������
*/
int main(int argc, char* argv[])
{
	/*��ʼ��WSA*/
	WSADATA wsaData;
	SYSTEM_INFO sysInfo;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "��ʼ��WSAʧ�ܡ�\n";
		return 1;
	}
	/*������ɶ˿ڶ���*/
	HANDLE hComPort;
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	/*��ȡϵͳ��Ϣ������CPU�����������߳�*/
	GetSystemInfo(&sysInfo);
	for (int i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
	{
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
	}
	/*�����ص��׽���*/
	SOCKET hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(8888);
	/*���׽���*/
	bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr));
	listen(hServSock, 5);
	int recvBytes, flags = 0;

	/*ѭ�����տͻ�������*/
	while (true)
	{	
		/*��ʼ���ͻ��˾���ṹ����*/
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);
		SOCKET hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &addrLen);
		ClntHandleData* handleInfo = (ClntHandleData*)malloc(sizeof(ClntHandleData));
		handleInfo->clntSock = hClntSock;
		memcpy(&(handleInfo->clntAddr), &clntAddr, addrLen);

		/*���ͻ����׽�������ɶ˿ڰ�*/
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		/*����IO���������ö�дģʽΪREAD���������Կͻ��˵���Ϣ*/
		IOData* ioInfo = (IOData*)malloc(sizeof(IOData));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = RW_MODE::READ;
		WSARecv(handleInfo->clntSock, &(ioInfo->wsaBuf), 1, (LPDWORD)&recvBytes, (LPDWORD)&flags, &(ioInfo->overlapped), NULL);

	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

/**
* @brief	IOCP�����߳�������
* @param	comPort		�ص��˿ڶ���
*
* @return	DWORD32
*/
DWORD32 WINAPI EchoThreadMain(LPVOID comPort_)
{
	/*����Ϊ��ɶ˿ڶ��󣬽�������ת��*/
	HANDLE hComPort = (HANDLE)comPort_;
	IOData* ioInfo;				/*IO���ݶ���ָ��*/
	ClntHandleData* handleInfo;	/*�ͻ��˾������ָ��*/
	DWORD bytesTrans;
	DWORD flags = 0;

	/*�����߳�ѭ������IO�����Ϣ*/
	while (true)
	{
		/*�ȴ�����ȡ����ɵ�IO��Ϣ�����û��������*/
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (PULONG_PTR)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		/*��ȡ�ͻ����׽���*/
		SOCKET clntSock = handleInfo->clntSock;
		if (ioInfo->rwMode == RW_MODE::READ)
		{
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->wsaBuf.buf[ioInfo->wsaBuf.len] = '\0';
			//	������ɵĶ�����
			if (bytesTrans == 0 || std::string(ioInfo->wsaBuf.buf) == "quit\n")	//	�ͻ��˶Ͽ����ӣ��ͷ���Դ
			{
				closesocket(clntSock);
				free(ioInfo);
				free(handleInfo);
				continue;
			}
			/*��ʼ��IO���ݶ����û�д����*/
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->rwMode = RW_MODE::WRITE;
			printf("�յ��ͻ���[%s:%d]����Ϣ��%s", inet_ntoa(*((in_addr*)&(handleInfo->clntAddr.sin_addr))), ntohs(handleInfo->clntAddr.sin_port), ioInfo->wsaBuf.buf);
			WSASend(clntSock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
			
			/*�½�һ��IO���ݶ���������һ�ζ�����*/
			ioInfo = (IOData*)malloc(sizeof(IOData));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = RW_MODE::READ;
			WSARecv(handleInfo->clntSock, &(ioInfo->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioInfo->overlapped), NULL);
		}
		else
		{
			//	������ɵ�д����
			free(ioInfo);
		}
	}
}


