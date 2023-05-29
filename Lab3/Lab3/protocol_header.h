/*
* @File: protocol_header.h
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/26 下午 8:01
* @Description: 定义了代表ip和tcp头部结构的结构体
* Version V1.0
*/
#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>


typedef struct IPHeader
{
	u_char ihl : 4,
		version : 4;
	u_char tos;
	u_short tot_len;
	u_short id;
	u_short flag_off;
	u_char ttl;
	u_char protocol;
	u_short checksum;
	u_long saddr;
	u_long daddr;
	friend std::ostream& operator<<(std::ostream&, const IPHeader&);
}IPHeader;


typedef struct TCPHeader
{
	u_short sport;
	u_short dport;
	u_int seq;
	u_int ack_seq;
	u_char res1 : 4,
		thl : 4;
	u_char flag;
	u_short win;
	u_short checksum;
	u_short urp;
	friend std::ostream& operator<<(std::ostream&, const TCPHeader&);
}TCPHeader;
