#include <cstdio>
#include <iostream>
#include <winsock2.h>
#include <string>
#pragma comment(lib, "ws2_32.lib") // 加载 ws2_32.dll
using namespace std;
int main()
{
    // 初始化 DLL
    WSADATA wsaData;                      // 存放被 WSAStartup 函数调用后返回的 Windows Sockets 数据
    WSAStartup(MAKEWORD(2, 2), &wsaData); // MAKEWORD(2, 2) 请求使用 2.2 版的 Winsock API

    // 创建套接字
    SOCKET servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // AF_INET: IPv4, SOCK_STREAM: TCP
    if (servSock == INVALID_SOCKET)
    {
        cout << "socket error !" << endl;
        return 0;
    }

    // 绑定套接字
    sockaddr_in sockAddr;                              // IPv4 套接字地址结构
    memset(&sockAddr, 0, sizeof(sockAddr));            // 每个字节都用 0 填充
    sockAddr.sin_family = AF_INET;                     // 使用 IPv4 地址
    sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 具体的 IP 地址
    sockAddr.sin_port = htons(9988);                   // 端口
    sockAddr.sin_addr.s_addr = INADDR_ANY;             // 任意 IP 地址
    // INADDR_ANY 表示不管是哪个网卡接收到数据，只要目的端口是 9988，就会被该应用程序接收到
    if (bind(servSock, (SOCKADDR *)&sockAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        cout << "bind error !" << endl;
        return 0;
    }

    // 监听套接字
    if (listen(servSock, 20) == SOCKET_ERROR)
    {
        cout << "listen error !" << endl;
        return 0;
    }

    // 循环接收数据
    SOCKET sClient;
    sockaddr_in remoteAddr;
    int nAddrlen = sizeof(remoteAddr);
    char revData[255];

    while (true)
    {
        cout << "等待连接..." << endl;
        sClient = accept(servSock, (SOCKADDR *)&remoteAddr, &nAddrlen);
        if (sClient == INVALID_SOCKET)
        {
            cout << "accept error !" << endl;
            continue;
        }
        cout << "接受到一个连接：" << inet_ntoa(remoteAddr.sin_addr) << endl;

        const char *sendStr = "Want to Get Client Name";
        send(sClient, sendStr, strlen(sendStr) + sizeof(char), 0); // 发送数据
        // 接收数据
        int ret = recv(sClient, revData, 255, 0);
        cout << "接收到数据：" << revData << endl;
        closesocket(sClient);
    }

    closesocket(servSock);
    // closesocket(sClient);
    WSACleanup();
    return 0;
}