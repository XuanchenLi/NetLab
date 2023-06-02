#include <iostream>
#include <WinSock2.h>
#include <mstcpip.h>
#include "FTPServer.h"

#pragma comment(lib, "Ws2_32.lib")


int main()
{
    try
    {
        FTPServer server("./home/");
    }
    catch (std::exception e)
    {
        puts(e.what());
    }
}

