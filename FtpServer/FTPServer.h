/*
* @File: FTPServer.h
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/29 下午 5:06
* @Description: 声明了FTP服务端所需的各种结构和FTP服务器类
* Version V1.0
*/
#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <io.h>
#include <filesystem>
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
	OVERLAPPED overlapped;	/*重叠IO所需对象*/
	WSABUF wsaBuf;			/*重叠IO缓冲对象*/
	char buffer[BUF_SIZE];	/*缓冲区*/
	RW_MODE rwMode;			/*读写模式*/
} IOData;


/**
* @brief	FTP服务器类
*/
class FTPServer
{
public :
	/**
	* @brief	服务器对象构造函数
	* @param	pDic	当前工作目录
	* @param	pPort	服务器端口
	*/
	FTPServer(std::string dic="./", u_short port=9999);
	/**
	* @brief	服务器对象析构函数，释放所有资源
	*/
	~FTPServer();

private:

	/**
	* @brief	代表终止线程的指令，线程接收到该消息时结束运行
	*/
	static const ULONG_PTR COMPLETION_KEY_SHUTDOWN;
	/**
	* @brief	从缓冲区中读取用户发送的指令和参数
	* @param	IOData		IO数据对象
	*
	* @return	std::pair<FTP_COMMAND, std::string>	指令和参数
	*/
	static std::pair<FTP_COMMAND, std::string> readCommand(const IOData*);
	/**
	* @brief	IOCP工作线程主函数
	* @param	LPVOID		函数参数
	*
	* @return	DWORD32
	*/
	static DWORD32 WINAPI ftpThreadMain(LPVOID);
	/**
	* @brief	封装传输文件的逻辑
	* @param	fName			文件名
	* @param	ClntHandleData	客户端句柄对象
	*
	*/
	static void transFile(std::string fName, const ClntHandleData&);
	/**
	* @brief	封装读取并保存文件的逻辑
	* @param	fName			文件名
	* @param	ClntHandleData	客户端句柄对象
	*
	*/
	static void readFile(std::string fName, const ClntHandleData&);
	/**
	* @brief	封装传输文件列表的逻辑
	* @param	list			文件信息列表
	* @param	ClntHandleData	客户端句柄对象
	*
	*/
	static void transList(const std::vector<_finddata_t>& list, const ClntHandleData&);
	/**
	* @brief	封装传输字符串消息的逻辑
	* @param	ClntHandleData*	客户端句柄对象指针
	* @param	string			消息
	* 
	*/
	static void transPrompt(ClntHandleData*, std::string);
	/**
	* @brief	新建一个IO数据对象
	* @param	RW_MODE		读写模式
	*
	*/
	static IOData* getNewBlock(RW_MODE);
	/**
	* @brief	处理GET指令
	* @param	ClntHandleData*	客户端句柄对象指针
	* @param	IOData*			IO数据对象指针			
	* @param	filePath		文件路径
	*
	*/
	static void handleGet(ClntHandleData*, IOData*, std::string filePath);
	/**
	* @brief	处理PUT指令
	* @param	ClntHandleData*	客户端句柄对象指针
	* @param	IOData*			IO数据对象指针
	* @param	filePath		文件路径
	*
	*/
	static void handlePut(ClntHandleData*, IOData*, std::string fileName);
	/**
	* @brief	处理LIST指令
	* @param	ClntHandleData*	客户端句柄对象指针
	* @param	IOData*			IO数据对象指针
	* @param	string			目录路径
	*
	*/
	static void handleList(ClntHandleData*, IOData*, std::string);
	/**
	* @brief	处理QUIT指令
	* @param	ClntHandleData*				客户端句柄对象指针
	* @param	IOData*						IO数据对象指针
	* @param	std::unordered_set<SOCKET>	FTP服务器维护的客户端句柄集合
	*
	*/
	static void handleQuit(ClntHandleData*, IOData*, std::unordered_set<SOCKET>&);
	/**
	* @brief	处理未识别的指令
	* @param	ClntHandleData*	客户端句柄对象指针
	* @param	IOData*			IO数据对象指针
	*
	*/
	static void handleUnknown(ClntHandleData*, IOData*);
	/**
	* @brief	获取文件信息列表
	* @param	string						目录路径
	*
	* @return	std::vector<_finddata_t>	文件信息列表
	*/
	static std::vector<_finddata_t> getFileInfoList(std::string);
	/**
	* @brief	判断文件是否可用
	* @param	string						文件路径
	*
	* @return	bool						True代表可用
	*/
	static bool isFileAvailable(std::string);

	std::string dic;						/*当前工作目录*/
	std::vector<HANDLE> workerHandles;		/*工作线程句柄列表*/
	std::unordered_set<SOCKET> clntSocks;	/*客户端句柄集合*/
	SOCKET hServerSock;						/*服务器套接字*/
	HANDLE hComPort;						/*完成端口对象*/
	SOCKADDR_IN servAddr;					/*服务器地址*/

};

