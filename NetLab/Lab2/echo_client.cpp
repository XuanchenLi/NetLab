#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")


#define BUF_SIZE 1024

char message[BUF_SIZE];

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("Usage: %s <IP> <Port>\n", argv[0]);
		exit(1);
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "��ʼ��WSAʧ�ܡ�\n";
		return 1;
	}

	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << "��ʼ���׽���ʧ�ܡ�\n";
		WSACleanup();
		return 1;
	}

	SOCKADDR_IN servAddr;
	memset(&servAddr, 0, sizeof(SOCKADDR_IN));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(argv[1]);
	servAddr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == -1) {
		std::cerr << "���ӷ�����ʧ�ܡ�\n";
		WSACleanup();
		return 1;
	}
	while (true)
	{
		fputs("������Ϣ��", stdout);
		fgets(message, BUF_SIZE, stdin);
		
		send(sock, message, strlen(message), MSG_WAITALL);
		int recvLen = recv(sock, message, BUF_SIZE - 1, MSG_WAITALL);
		message[recvLen] = '\0';
		if (recvLen == 0) break;
		printf("�յ��������Ϣ��%s", message);
	}
	closesocket(sock);
	WSACleanup();
	return 0;

}