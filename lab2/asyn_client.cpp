#include <winsock2.h>
#include <iostream>
#include <string>
#include <ctime>
#include <windows.h>
#include <cstring>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

int main()
{
    // 初始化Winsock库，指定版本为2.2
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(sockVersion, &data) != 0)
    {
        return 0;
    }

    // 创建一个TCP套接字
    SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sclient == INVALID_SOCKET)
    {
        cout << "invalid socket!" << endl;
        return 0;
    }

    // 设置服务器地址和端口
    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(9988);
    serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    // 连接到服务器
    if (connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        // 连接失败
        cout << "connect error !" << endl;
        closesocket(sclient);
        return 0;
    }
    int i = 5;
    while (i--)
    {
        string data;
        time_t now = time(0);
        char *dt = ctime(&now);
        WSABUF buf;
        buf.buf = dt;
        buf.len = strlen(dt);
        // char * sendData = "你好，TCP服务端，我是客户端\n";
        WSABUF *pBuf = &buf;
        DWORD dwSendBytes = 0;
        DWORD dwFlags = 0;
        WSAOVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(WSAOVERLAPPED));
        overlapped.hEvent = WSACreateEvent();
        WSASend(sclient, pBuf, 1, &dwSendBytes, dwFlags, &overlapped, NULL);
        // send(sclient, dt, strlen(dt), 0);
        char recData[255];
        // int ret = recv(sclient, recData, 255, 0);
        // if (ret > 0)
        // {
        //     recData[ret] = 0x00;
        //     cout << recData << endl;
        // }
    }

    closesocket(sclient);
    WSACleanup();
    system("pause");
    return 0;
}