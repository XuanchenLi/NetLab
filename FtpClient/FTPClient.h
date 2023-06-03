/*
* @File: FTPClient.h
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/6/1 下午 4:31
* @Description: 声明了FTP客户端所需各种结构和FTP客户端类
* Version V1.0
*/
#pragma once
#include <string>
#include <vector>
#include <utility>
#include <unordered_set>
#include <WinSock2.h>

#define BUF_SIZE 4096
/*
* @brief	区分异步读和异步写操作
*/
enum class RW_MODE
{
	READ,
	WRITE
};
/*
* @brief	FTP支持的各种指令
*/
enum class FTP_COMMAND
{
	QUIT,		/*断开连接*/
	GET,		/*下载文件*/
	PUT,		/*上传文件*/
	LIST,		/*列出文件信息*/
	UNKNOWN		/*未知指令*/
};


/**
* @brief	FTP客户端类
*/
class FTPClient
{
public:
	/**
	* @brief	客户端对象构造函数
	* @param	dir	当前工作目录
	* @param	servIP	服务器IP
	* @param	servPort	服务器端口
	*/
	FTPClient(std::string dir = "./", std::string servIP = "127.0.0.1", u_short servPort = 9999);
	/**
	* @brief	客户端对象析构函数，释放所有资源
	*/
	~FTPClient();

private:
	/**
	* @brief	从缓冲区中读取用户发送的指令和参数
	*
	* @return	std::pair<FTP_COMMAND, std::string>	指令和参数
	*/
	std::pair<FTP_COMMAND, std::string> readCommand();
	/**
	* @brief	封装传输文件的逻辑
	* @param	fName			文件名
	*
	*/
	void transFile(std::string);
	/**
	* @brief	封装读取并保存文件的逻辑
	* @param	fName			文件名
	*
	*/
	void readFile(std::string);
	/**
	* @brief	判断文件是否可用
	* @param	string						文件路径
	*
	* @return	bool						True代表可用
	*/
	bool isFileAvailable(std::string);
	/**
	* @brief	处理未识别的指令
	*
	* @return	std::vector<_finddata_t>	文件信息列表
	*/
	void handleUnknown();
	/**
	* @brief	处理QUIT指令
	*
	*/
	void handleQuit();
	/**
	* @brief	处理LIST指令
	*
	*/
	void handleList();
	/**
	* @brief	处理PUT指令
	* @param	filePath		文件路径
	*
	*/
	void handlePut(std::string);
	/**
	* @brief	处理GET指令
	* @param	filePath		文件路径
	*
	*/
	void handleGet(std::string);

	std::string dic;			/*当前工作目录*/
	SOCKET clntSock;			/*客户端套接字*/
	SOCKADDR_IN servAddr;		/*服务器地址*/
	char* buffer;				/*缓冲区指针*/
};