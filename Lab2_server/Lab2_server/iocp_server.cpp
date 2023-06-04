/*
* @File: iocp_server.cpp
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/27 下午 7:00
* @Description: 定义了IOCP回声服务端所需的各种结构和运行逻辑，
*				服务器不断接收客户端消息并将其返回。
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
* @brief	区分异步读和异步写操作
*/
enum class RW_MODE
{
	READ,
	WRITE
};

/**
* @brief	封装客户端句柄信息
*/
typedef struct
{
	SOCKET clntSock;		/*客户端套接字*/
	SOCKADDR_IN clntAddr;	/*客户端地址*/
} ClntHandleData;

/**
* @brief	封装IO包数据
*/
typedef struct
{
	OVERLAPPED overlapped;		/*重叠IO所需对象*/
	WSABUF wsaBuf;				/*重叠IO缓冲区对象*/
	char buffer[BUF_SIZE];		/*缓冲区*/
	RW_MODE rwMode;				/*读写模式*/
} IOData;

/**
* @brief	IOCP工作线程主函数
* @param	comPort		重叠端口对象
* 
* @return	DWORD32
*/
DWORD32 WINAPI EchoThreadMain(LPVOID comPort);


/**
* @brief 主函数
*/
int main(int argc, char* argv[])
{
	/*初始化WSA*/
	WSADATA wsaData;
	SYSTEM_INFO sysInfo;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "初始化WSA失败。\n";
		return 1;
	}
	/*创建完成端口对象*/
	HANDLE hComPort;
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	/*获取系统信息，创建CPU核数个工作线程*/
	GetSystemInfo(&sysInfo);
	for (int i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
	{
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
	}
	/*创建重叠套接字*/
	SOCKET hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(8888);
	/*绑定套接字*/
	bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr));
	listen(hServSock, 5);
	int recvBytes, flags = 0;

	/*循环接收客户端连接*/
	while (true)
	{	
		/*初始化客户端句柄结构对象*/
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);
		SOCKET hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &addrLen);
		ClntHandleData* handleInfo = (ClntHandleData*)malloc(sizeof(ClntHandleData));
		handleInfo->clntSock = hClntSock;
		memcpy(&(handleInfo->clntAddr), &clntAddr, addrLen);

		/*将客户端套接字与完成端口绑定*/
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		/*创建IO包对象，设置读写模式为READ，接收来自客户端的消息*/
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
* @brief	IOCP工作线程主函数
* @param	comPort		重叠端口对象
*
* @return	DWORD32
*/
DWORD32 WINAPI EchoThreadMain(LPVOID comPort_)
{
	/*参数为完成端口对象，进行类型转换*/
	HANDLE hComPort = (HANDLE)comPort_;
	IOData* ioInfo;				/*IO数据对象指针*/
	ClntHandleData* handleInfo;	/*客户端句柄对象指针*/
	DWORD bytesTrans;
	DWORD flags = 0;

	/*工作线程循环处理IO完成消息*/
	while (true)
	{
		/*等待并获取已完成的IO消息，如果没有则阻塞*/
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (PULONG_PTR)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		/*获取客户端套接字*/
		SOCKET clntSock = handleInfo->clntSock;
		if (ioInfo->rwMode == RW_MODE::READ)
		{
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->wsaBuf.buf[ioInfo->wsaBuf.len] = '\0';
			//	处理完成的读操作
			if (bytesTrans == 0 || std::string(ioInfo->wsaBuf.buf) == "quit\n")	//	客户端断开连接，释放资源
			{
				closesocket(clntSock);
				free(ioInfo);
				free(handleInfo);
				continue;
			}
			/*初始化IO数据对象用户写操作*/
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->rwMode = RW_MODE::WRITE;
			printf("收到客户端[%s:%d]的信息：%s", inet_ntoa(*((in_addr*)&(handleInfo->clntAddr.sin_addr))), ntohs(handleInfo->clntAddr.sin_port), ioInfo->wsaBuf.buf);
			WSASend(clntSock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
			
			/*新建一个IO数据对象用于下一次读操作*/
			ioInfo = (IOData*)malloc(sizeof(IOData));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = RW_MODE::READ;
			WSARecv(handleInfo->clntSock, &(ioInfo->wsaBuf), 1, NULL, (LPDWORD)&flags, &(ioInfo->overlapped), NULL);
		}
		else
		{
			//	处理完成的写操作
			free(ioInfo);
		}
	}
}


