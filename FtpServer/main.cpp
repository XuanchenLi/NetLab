/*
* @File: main.cpp
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/5/29 下午 5:05
* @Description: 定义了FTP服务器主函数
* Version V1.0
*/
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

