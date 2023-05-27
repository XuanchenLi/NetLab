/*
* @File: main.cpp
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/26 下午 8:00
* @Description: 实验三主函数 
* Version V1.0
*/
#include <stdio.h>
#include <WinSock2.h>
#include <mstcpip.h>
#include <WS2tcpip.h>
#include "protocol_header.h"
#pragma comment(lib, "Ws2_32.lib")

#define BUF_SIZE 65536

char buffer[BUF_SIZE];

int main(int argc, char *argv[])
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "初始化Winsock失败。\n";
		return 1;
	}

	SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << "创建原始套接字失败。\n";
		WSACleanup();
		return 1;
	}

	DWORD optval = true;
	if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR)
	{
		std::cerr << "设置套接字属性失败。Error: " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	sockaddr_in localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	localAddr.sin_port = 8888;
	if (bind(sock, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
	{
		std::cerr << "绑定套接字失败。\n";
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	DWORD mode = 1;
	char charRecv[10];
	DWORD dwBytesRet = 0;
	if (WSAIoctl(sock, (IOC_IN | IOC_VENDOR | 1), &mode, sizeof(mode), charRecv, sizeof(charRecv), &dwBytesRet, NULL, NULL) == SOCKET_ERROR)
	{
		std::cerr << "设置混杂模式失败。\n";
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	IPHeader ipHdr;
	TCPHeader tcpHdr;

	while (true)
	{
		int dataSize = recv(sock, buffer, sizeof(buffer), 0);
		if (dataSize > 0)
		{
			ipHdr = *(IPHeader*)buffer;
			std::cout << "----------------------------------------IP首部：\n" << ipHdr;
			if (ipHdr.protocol == IPPROTO_TCP)
			{
				tcpHdr = *(TCPHeader*)(buffer + ipHdr.ihl * 4);
			}
			std::cout << "----------------------------------------TCP首部：\n" << tcpHdr;
			char* dataPtr = buffer + (ipHdr.ihl * 4) + (tcpHdr.thl * 4);
			int dataLen = dataSize - (ipHdr.ihl * 4) - (tcpHdr.thl * 4);
			std::cout << "----------------------------------------TCP数据：\n";
			for (int i = 0; i < dataLen; ++i) printf("%c", dataPtr[i]);
			puts("");
		}
		else if (dataSize == SOCKET_ERROR)
		{
			printf("Error: %d\n", WSAGetLastError());
			break;
		}
	}
	closesocket(sock);
	WSACleanup();
	return 0;
}


