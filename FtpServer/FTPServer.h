/*
* @File: FTPServer.h
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/29 ���� 5:06
* @Description: ������FTP���������ĸ��ֽṹ��FTP��������
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
* @brief	�����첽�����첽д����
*/
enum class RW_MODE
{
	READ,
	WRITE
};
/*
* @brief	FTP֧�ֵĸ���ָ��
*/
enum class FTP_COMMAND
{
	QUIT,		/*�Ͽ�����*/
	GET,		/*�����ļ�*/
	PUT,		/*�ϴ��ļ�*/
	LIST,		/*�г��ļ���Ϣ*/
	UNKNOWN		/*δָ֪��*/
};

/**
* @brief	��װ�ͻ��˾����Ϣ
*/
typedef struct
{
	SOCKET clntSock;		/*�ͻ����׽���*/
	SOCKADDR_IN clntAddr;	/*�ͻ��˵�ַ*/
} ClntHandleData;

/**
* @brief	��װIO������
*/
typedef struct
{
	OVERLAPPED overlapped;	/*�ص�IO�������*/
	WSABUF wsaBuf;			/*�ص�IO�������*/
	char buffer[BUF_SIZE];	/*������*/
	RW_MODE rwMode;			/*��дģʽ*/
} IOData;


/**
* @brief	FTP��������
*/
class FTPServer
{
public :
	/**
	* @brief	�����������캯��
	* @param	pDic	��ǰ����Ŀ¼
	* @param	pPort	�������˿�
	*/
	FTPServer(std::string dic="./", u_short port=9999);
	/**
	* @brief	���������������������ͷ�������Դ
	*/
	~FTPServer();

private:

	/**
	* @brief	������ֹ�̵߳�ָ��߳̽��յ�����Ϣʱ��������
	*/
	static const ULONG_PTR COMPLETION_KEY_SHUTDOWN;
	/**
	* @brief	�ӻ������ж�ȡ�û����͵�ָ��Ͳ���
	* @param	IOData		IO���ݶ���
	*
	* @return	std::pair<FTP_COMMAND, std::string>	ָ��Ͳ���
	*/
	static std::pair<FTP_COMMAND, std::string> readCommand(const IOData*);
	/**
	* @brief	IOCP�����߳�������
	* @param	LPVOID		��������
	*
	* @return	DWORD32
	*/
	static DWORD32 WINAPI ftpThreadMain(LPVOID);
	/**
	* @brief	��װ�����ļ����߼�
	* @param	fName			�ļ���
	* @param	ClntHandleData	�ͻ��˾������
	*
	*/
	static void transFile(std::string fName, const ClntHandleData&);
	/**
	* @brief	��װ��ȡ�������ļ����߼�
	* @param	fName			�ļ���
	* @param	ClntHandleData	�ͻ��˾������
	*
	*/
	static void readFile(std::string fName, const ClntHandleData&);
	/**
	* @brief	��װ�����ļ��б���߼�
	* @param	list			�ļ���Ϣ�б�
	* @param	ClntHandleData	�ͻ��˾������
	*
	*/
	static void transList(const std::vector<_finddata_t>& list, const ClntHandleData&);
	/**
	* @brief	��װ�����ַ�����Ϣ���߼�
	* @param	ClntHandleData*	�ͻ��˾������ָ��
	* @param	string			��Ϣ
	* 
	*/
	static void transPrompt(ClntHandleData*, std::string);
	/**
	* @brief	�½�һ��IO���ݶ���
	* @param	RW_MODE		��дģʽ
	*
	*/
	static IOData* getNewBlock(RW_MODE);
	/**
	* @brief	����GETָ��
	* @param	ClntHandleData*	�ͻ��˾������ָ��
	* @param	IOData*			IO���ݶ���ָ��			
	* @param	filePath		�ļ�·��
	*
	*/
	static void handleGet(ClntHandleData*, IOData*, std::string filePath);
	/**
	* @brief	����PUTָ��
	* @param	ClntHandleData*	�ͻ��˾������ָ��
	* @param	IOData*			IO���ݶ���ָ��
	* @param	filePath		�ļ�·��
	*
	*/
	static void handlePut(ClntHandleData*, IOData*, std::string fileName);
	/**
	* @brief	����LISTָ��
	* @param	ClntHandleData*	�ͻ��˾������ָ��
	* @param	IOData*			IO���ݶ���ָ��
	* @param	string			Ŀ¼·��
	*
	*/
	static void handleList(ClntHandleData*, IOData*, std::string);
	/**
	* @brief	����QUITָ��
	* @param	ClntHandleData*				�ͻ��˾������ָ��
	* @param	IOData*						IO���ݶ���ָ��
	* @param	std::unordered_set<SOCKET>	FTP������ά���Ŀͻ��˾������
	*
	*/
	static void handleQuit(ClntHandleData*, IOData*, std::unordered_set<SOCKET>&);
	/**
	* @brief	����δʶ���ָ��
	* @param	ClntHandleData*	�ͻ��˾������ָ��
	* @param	IOData*			IO���ݶ���ָ��
	*
	*/
	static void handleUnknown(ClntHandleData*, IOData*);
	/**
	* @brief	��ȡ�ļ���Ϣ�б�
	* @param	string						Ŀ¼·��
	*
	* @return	std::vector<_finddata_t>	�ļ���Ϣ�б�
	*/
	static std::vector<_finddata_t> getFileInfoList(std::string);
	/**
	* @brief	�ж��ļ��Ƿ����
	* @param	string						�ļ�·��
	*
	* @return	bool						True�������
	*/
	static bool isFileAvailable(std::string);

	std::string dic;						/*��ǰ����Ŀ¼*/
	std::vector<HANDLE> workerHandles;		/*�����߳̾���б�*/
	std::unordered_set<SOCKET> clntSocks;	/*�ͻ��˾������*/
	SOCKET hServerSock;						/*�������׽���*/
	HANDLE hComPort;						/*��ɶ˿ڶ���*/
	SOCKADDR_IN servAddr;					/*��������ַ*/

};

