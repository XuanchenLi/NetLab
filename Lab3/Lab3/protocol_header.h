/*
* @File: protocol_header.h
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/26 ���� 8:01
* @Description: �����˴���ip��tcpͷ���ṹ�Ľṹ��
* Version V1.0
*/
#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

/*
* @brief IPͷ�ṹ
* 
*/
typedef struct IPHeader
{
	u_char ihl : 4,		/*�ײ�����*/
		version : 4;	/*�汾*/
	u_char tos;			/*��������*/
	u_short tot_len;	/*�ܳ���*/
	u_short id;			/*��ʶ*/
	u_short flag_off;	/*ƫ�����Ͷ�ƫ����*/
	u_char ttl;			/*����ʱ��*/
	u_char protocol;	/*Э������*/
	u_short checksum;	/*У���*/
	u_long saddr;		/*Դ��ַ*/
	u_long daddr;		/*Ŀ�ĵ�ַ*/
	/*
	* @brief	��ʽ�����IPͷ
	* @param	os		�������������
	* @param	ipHdr	IPͷ�ṹ
	* 
	* @return	���������
	*/
	friend std::ostream& operator<<(std::ostream&, const IPHeader&);
}IPHeader;

/*
* @brief TCPͷ�ṹ
*
*/
typedef struct TCPHeader
{
	u_short sport;		/*Դ�˿�*/
	u_short dport;		/*Ŀ�Ķ˿�*/
	u_int seq;			/*���*/
	u_int ack_seq;		/*ȷ�����*/
	u_char res1 : 4,	/*����*/
		thl : 4;		/*�ײ�����*/
	u_char flag;		/*��־*/
	u_short win;		/*���ڴ�С*/
	u_short checksum;	/*У���*/
	u_short urp;		/*����ָ��*/
	/*
	* @brief	��ʽ�����TCPͷ
	* @param	os		�������������
	* @param	tcpHdr	TCPͷ�ṹ
	*
	* @return	���������
	*/
	friend std::ostream& operator<<(std::ostream&, const TCPHeader&);
}TCPHeader;
