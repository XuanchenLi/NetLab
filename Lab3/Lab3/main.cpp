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

/*缓冲区大小*/
#define BUF_SIZE 65536

/*缓冲区*/
char buffer[BUF_SIZE];

/*主函数*/
int main(int argc, char *argv[])
{
	WSADATA wsaData;
	/*初始化WSA*/
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "初始化Winsock失败。\n";
		return 1;
	}
	/*创建原始套接字，选项为SOCK_RAW*/
	SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << "创建原始套接字失败。\n";
		WSACleanup();
		return 1;
	}
	/*绑定套接字地址*/
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
	/*设置为混杂模式*/
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

	/*循环接收数据包*/
	while (true)
	{
		int dataSize = recv(sock, buffer, sizeof(buffer), 0);
		if (dataSize > 0)
		{
			/*创建IP头*/
			ipHdr = *(IPHeader*)buffer;
			/*过滤非TCP包，创建TCP头*/
			if (ipHdr.protocol == IPPROTO_TCP)
			{
				tcpHdr = *(TCPHeader*)(buffer + ipHdr.ihl * 4);
			}
			else
			{
				continue;
			}
			/*过滤数据包IP和端口*/
			if ((strcmp(inet_ntoa(*(struct in_addr*)&ipHdr.daddr), "127.0.0.1") != 0 ) && (strcmp(inet_ntoa(*(struct in_addr*)&ipHdr.saddr), "127.0.0.1") != 0)
				|| (ntohs(tcpHdr.sport) != 8888 && ntohs(tcpHdr.dport) != 8888))
				continue;
			/*输出首部信息*/
			std::cout << "----------------------------------------IP首部：\n" << ipHdr;
			std::cout << "----------------------------------------TCP首部：\n" << tcpHdr;
			/*计算数据部分偏移*/
			char* dataPtr = buffer + (ipHdr.ihl * 4) + (tcpHdr.thl * 4);
			/*计算数据部分长度*/
			int dataLen = dataSize - (ipHdr.ihl * 4) - (tcpHdr.thl * 4);
			/*输出数据*/
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


