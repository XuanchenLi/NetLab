/*
* @File: echo_client.cpp
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/27 下午 8:00
* @Description: 定义了客户端运行逻辑
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

/*缓冲区*/
char message[BUF_SIZE];

int main(int argc, char* argv[])
{
	/*初始化WSA*/
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "初始化WSA失败。\n";
		return 1;
	}
	/*初始化套接字*/
	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << "初始化套接字失败。\n";
		WSACleanup();
		return 1;
	}

	SOCKADDR_IN servAddr;
	memset(&servAddr, 0, sizeof(SOCKADDR_IN));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAddr.sin_port = htons(8888);
	/*连接服务器*/
	if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == -1) {
		std::cerr << "连接服务器失败。\n";
		WSACleanup();
		return 1;
	}
	/*循环发送、接收消息*/
	while (true)
	{
		fputs("输入信息：", stdout);
		fgets(message, BUF_SIZE, stdin);
		if (std::string(message) == "quit\n")
		{
			printf("12312");
			break;
		}
		send(sock, message, strlen(message), 0);
		int recvLen = recv(sock, message, BUF_SIZE - 1, 0);
		message[recvLen] = '\0';
		if (recvLen == 0) break;
		printf("收到服务端消息：%s", message);
	}
	closesocket(sock);
	WSACleanup();
	return 0;

}