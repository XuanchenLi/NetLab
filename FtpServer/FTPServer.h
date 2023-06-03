#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <io.h>
#include <filesystem>
#include <WinSock2.h>

#define BUF_SIZE 4096

enum class RW_MODE
{
	READ,
	WRITE
};

enum class FTP_COMMAND
{
	QUIT,
	GET,
	PUT,
	LIST,
	UNKNOWN
};

typedef struct
{
	SOCKET clntSock;
	SOCKADDR_IN clntAddr;
} ClntHandleData;

typedef struct
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	RW_MODE rwMode;
} IOData;

class FTPServer
{
public :
	FTPServer(std::string dic="./", u_short port=9999);
	~FTPServer();

private:
	static const ULONG_PTR COMPLETION_KEY_SHUTDOWN;
	static std::pair<FTP_COMMAND, std::string> readCommand(const IOData*);
	static DWORD32 WINAPI ftpThreadMain(LPVOID);
	static void transFile(std::string fName, const ClntHandleData&);
	static void readFile(std::string fName, const ClntHandleData&);
	static void transList(const std::vector<_finddata_t>&, const ClntHandleData&);
	static void transPrompt(ClntHandleData*, std::string);
	static IOData* getNewBlock(RW_MODE);
	static void handleGet(ClntHandleData*, IOData*, std::string filePath);
	static void handlePut(ClntHandleData*, IOData*, std::string fileName);
	static void handleList(ClntHandleData*, IOData*, std::string);
	static void handleQuit(ClntHandleData*, IOData*, std::unordered_set<SOCKET>&);
	static void handleUnknown(ClntHandleData*, IOData*);
	static std::vector<_finddata_t> getFileInfoList(std::string);
	static bool isFileAvailable(std::string);

	std::string dic;
	std::vector<HANDLE> workerHandles;
	std::unordered_set<SOCKET> clntSocks;
	SOCKET hServerSock;
	HANDLE hComPort;
	SOCKADDR_IN servAddr;

};

