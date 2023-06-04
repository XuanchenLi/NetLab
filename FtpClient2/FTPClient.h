/*
* @File: FTPClient.h
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/6/1 ���� 4:31
* @Description: ������FTP�ͻ���������ֽṹ��FTP�ͻ�����
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
* @brief	FTP�ͻ�����
*/
class FTPClient
{
public:
	/**
	* @brief	�ͻ��˶����캯��
	* @param	dir	��ǰ����Ŀ¼
	* @param	servIP	������IP
	* @param	servPort	�������˿�
	*/
	FTPClient(std::string dir = "./", std::string servIP = "127.0.0.1", u_short servPort = 9999);
	/**
	* @brief	�ͻ��˶��������������ͷ�������Դ
	*/
	~FTPClient();

private:
	/**
	* @brief	�ӻ������ж�ȡ�û����͵�ָ��Ͳ���
	*
	* @return	std::pair<FTP_COMMAND, std::string>	ָ��Ͳ���
	*/
	std::pair<FTP_COMMAND, std::string> readCommand();
	/**
	* @brief	��װ�����ļ����߼�
	* @param	fName			�ļ���
	*
	*/
	void transFile(std::string);
	/**
	* @brief	��װ��ȡ�������ļ����߼�
	* @param	fName			�ļ���
	*
	*/
	void readFile(std::string);
	/**
	* @brief	�ж��ļ��Ƿ����
	* @param	string						�ļ�·��
	*
	* @return	bool						True�������
	*/
	bool isFileAvailable(std::string);
	/**
	* @brief	����δʶ���ָ��
	*
	* @return	std::vector<_finddata_t>	�ļ���Ϣ�б�
	*/
	void handleUnknown();
	/**
	* @brief	����QUITָ��
	*
	*/
	void handleQuit();
	/**
	* @brief	����LISTָ��
	*
	*/
	void handleList();
	/**
	* @brief	����PUTָ��
	* @param	filePath		�ļ�·��
	*
	*/
	void handlePut(std::string);
	/**
	* @brief	����GETָ��
	* @param	filePath		�ļ�·��
	*
	*/
	void handleGet(std::string);

	std::string dic;			/*��ǰ����Ŀ¼*/
	SOCKET clntSock;			/*�ͻ����׽���*/
	SOCKADDR_IN servAddr;		/*��������ַ*/
	char* buffer;				/*������ָ��*/
};