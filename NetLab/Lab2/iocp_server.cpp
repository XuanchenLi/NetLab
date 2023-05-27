#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>


#pragma comment(lib, "Ws2_32.lib")

#define BUF_SIZE 1024

enum class RW_MODE
{
	READ,
	WRITE
};

typedef struct
{
	SOCKET clntSock;
	SOCKADDR_IN clntAddr;
} ClntHandleData;

typedef struct
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	RW_MODE rwMode;
} IOData;

DWORD32 WINAPI EchoThreadMain(LPVOID comPort);


int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SYSTEM_INFO sysInfo;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "³õÊ¼»¯WSAÊ§°Ü¡£\n";
		return 1;
	}

	HANDLE hComPort;
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&sysInfo);
	for (int i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
	{
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
	}

	SOCKET hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(atoi(argv[1]));

	bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr));
	listen(hServSock, 5);
	int recvBytes, flags = 0;
	while (true)
	{	
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);
		SOCKET hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &addrLen);
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
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

DWORD32 WINAPI EchoThreadMain(LPVOID comPort_)
{
	HANDLE hComPort = (HANDLE)comPort_;
	IOData* ioInfo;
	ClntHandleData* handleInfo;
	DWORD bytesTrans;
	DWORD flags = 0;
	
	while (true)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (PULONG_PTR)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		SOCKET clntSock = handleInfo->clntSock;
		if (ioInfo->rwMode == RW_MODE::READ)
		{
			if (bytesTrans == 0)
			{
				closesocket(clntSock);
				free(ioInfo);
				free(handleInfo);
				continue;
			}
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = RW_MODE::WRITE;
			WSASend(clntSock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
			
			ioInfo = (IOData*)malloc(sizeof(IOData));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = RW_MODE::READ;
			WSARecv(handleInfo->clntSock, &(ioInfo->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioInfo->overlapped), NULL);
		}
		else
		{
			free(ioInfo);
		}
	}
}


