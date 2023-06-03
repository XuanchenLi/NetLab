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

/*
* @brief IP头结构
* 
*/
typedef struct IPHeader
{
	u_char ihl : 4,		/*首部长度*/
		version : 4;	/*版本*/
	u_char tos;			/*服务类型*/
	u_short tot_len;	/*总长度*/
	u_short id;			/*标识*/
	u_short flag_off;	/*偏移量和段偏移量*/
	u_char ttl;			/*生存时间*/
	u_char protocol;	/*协议类型*/
	u_short checksum;	/*校验和*/
	u_long saddr;		/*源地址*/
	u_long daddr;		/*目的地址*/
	/*
	* @brief	格式化输出IP头
	* @param	os		输出流对象引用
	* @param	ipHdr	IP头结构
	* 
	* @return	输出流对象
	*/
	friend std::ostream& operator<<(std::ostream&, const IPHeader&);
}IPHeader;

/*
* @brief TCP头结构
*
*/
typedef struct TCPHeader
{
	u_short sport;		/*源端口*/
	u_short dport;		/*目的端口*/
	u_int seq;			/*序号*/
	u_int ack_seq;		/*确认序号*/
	u_char res1 : 4,	/*保留*/
		thl : 4;		/*首部长度*/
	u_char flag;		/*标志*/
	u_short win;		/*窗口大小*/
	u_short checksum;	/*校验和*/
	u_short urp;		/*紧急指针*/
	/*
	* @brief	格式化输出TCP头
	* @param	os		输出流对象引用
	* @param	tcpHdr	TCP头结构
	*
	* @return	输出流对象
	*/
	friend std::ostream& operator<<(std::ostream&, const TCPHeader&);
}TCPHeader;
