/*
* @File: protocol_header.h
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/26 下午 8:01
* @Description: 定义了输出ip和tcp头部结构的方法
* Version V1.0
*/
#include "protocol_header.h"

/*
* @brief	格式化输出IP头
* @param	os		输出流对象引用
* @param	ipHdr	IP头结构
* 
* @return	输出流对象
*/
std::ostream& operator<<(std::ostream& os, const IPHeader& ipHdr)
{
	os << "Version: " << ipHdr.version * 1 << std::endl;
	os << "Header Length: " << ipHdr.ihl * 4 << " bytes\n";
	os << "Total Length: " << ipHdr.tot_len << " bytes\n";
	os << "Identification: " << ipHdr.id << std::endl;
	os << "Flags: " << "0x" << std::hex << (ipHdr.flag_off >> 13) << std::dec << std::endl;
	os << "Fragment offset: " << (ipHdr.flag_off & 0x1FFF) * 8 << " bytes\n";
	os << "Time to live: " << ipHdr.ttl * 1<< std::endl;
	os << "Protocol: " << ipHdr.protocol * 1<< std::endl;
	os << "Checksum: " << "0x" << std::hex << ipHdr.checksum << std::dec << std::endl;
	os << "Source: " << inet_ntoa(*((in_addr*)&ipHdr.saddr)) << std::endl;
	os << "Destination: " << inet_ntoa(*((in_addr*)&ipHdr.saddr)) << std::endl;
	return os;
}

/*
* @brief	格式化输出TCP头
* @param	os		输出流对象引用
* @param	tcpHdr	TCP头结构
*
* @return	输出流对象
*/
std::ostream& operator<<(std::ostream& os, const TCPHeader& tcpHdr)
{
	os << "Source port: " << ntohs(tcpHdr.sport) << std::endl;
	os << "Destination port: " << ntohs(tcpHdr.dport) << std::endl;
	os << "Sequence: " << tcpHdr.seq << std::endl;
	os << "Ack Sequence: " << tcpHdr.ack_seq << std::endl;
	os << "Header Length: " << tcpHdr.thl * 4 << " bytes\n";
	os << "Flags: " << "0x" << std::hex << tcpHdr.flag * 1<< std::dec << std::endl;
	os << "Window size: " << tcpHdr.win << std::endl;
	os << "Checksum: " << "0x" << std::hex << tcpHdr.checksum << std::dec << std::endl;
	os << "Urgent pointer: " << tcpHdr.urp << std::endl;
	return os;
}