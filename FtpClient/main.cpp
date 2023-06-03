/*
* @File: mian.cpp
* @Author: Xuanchen Li
* @Email: lixc5520@mails.jlu.edu.cn
* @Date: 2023/6/1 下午 4:31
* @Description: 定义了FTP客户端主函数
* Version V1.0
*/
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

