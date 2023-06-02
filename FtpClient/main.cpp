#include <iostream>
#include <WinSock2.h>
#include <mstcpip.h>
#include "FTPClient.h"

#pragma comment(lib, "Ws2_32.lib")


int main()
{
    try
    {
        FTPClient client("./home/");
    }
    catch (std::exception e)
    {
        puts(e.what());
    }
}

