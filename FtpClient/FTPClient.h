#pragma once
#include <string>
#include <vector>
#include <utility>
#include <unordered_set>
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


class FTPClient
{
public:
	FTPClient(std::string dir = "./", std::string servIP = "127.0.0.1", u_short servPort = 9999);
	~FTPClient();

private:
	std::pair<FTP_COMMAND, std::string> readCommand();
	void transFile(std::string);
	void readFile(std::string);
	bool isFileAvailable(std::string);
	void handleUnknown();
	void handleQuit();
	void handleList();
	void handlePut(std::string);
	void handleGet(std::string);

	std::string dic;
	SOCKET clntSock;
	SOCKADDR_IN servAddr;
	char* buffer;
};