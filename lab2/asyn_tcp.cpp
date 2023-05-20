#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <winsock2.h>

#define REQUEST_SUCCEEDED 0
#define INVALID_ARGUMENT 2
#define METHOD_NOT_DEFINED 1

typedef uint8_t ERROR_CODE;

using namespace std;

class Server
{
public:
    Server(int port)
    {
        WSADATA wsaData;
        WORD sockVersion = MAKEWORD(2, 2);          // MAKEWORD(2, 2) 请求使用 2.2 版的 Winsock API
                                                    // 存放被 WSAStartup 函数调用后返回的 Windows Sockets 数据
        if (WSAStartup(sockVersion, &wsaData) != 0) // 初始化 DLL
        {
            cout << "WSAStartup error !" << endl; // WSAStartup 函数用于初始化 Winsock DLL
            exit(1);
        }

        // 创建套接字
        SOCKET servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // AF_INET: IPv4, SOCK_STREAM: TCP
        if (servSock == INVALID_SOCKET)
        {
            cout << "socket error !" << endl;
            exit(1);
        }
        sockaddr_in sockAddr; // IPv4 套接字地址结构

        sockAddr.sin_family = AF_INET;                                               // 使用 IPv4 地址
        sockAddr.sin_addr.s_addr = INADDR_ANY;                                       // 任意 IP 地址
        sockAddr.sin_port = htons(port);                                             // 端口
        if (bind(servSock, (SOCKADDR *)&sockAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) // 绑定套接字
        {
            cout << "bind error !" << endl;
            exit(1);
        }

        if (listen(servSock, 20) == SOCKET_ERROR)
        {
            cout << "listen error !" << endl;
            exit(1);
        }

        SOCKET sClient;
        sockaddr_in remoteAddr;
        int nAddrlen = sizeof(remoteAddr);
        while (true)
        {
            cout << "waiting for connection" << endl;
            sClient = accept(servSock, (SOCKADDR *)&remoteAddr, &nAddrlen);
            if (sClient == INVALID_SOCKET)
            {
                cout << "accept error !" << endl;
                continue;
            }
            cout << "accept a connection: " << inet_ntoa(remoteAddr.sin_addr) << endl;

            CreateThreadToHandleRequest(sClient);
        }
        closesocket(servSock);
        WSACleanup();
    }

private:
    void CreateChildProcessToHandleRequest(SOCKET socket)
    {
        // 声明启动信息和进程信息的结构体
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        // 定义一个字符数组来存储句柄
        TCHAR lpHandle[20];

        // 打印socket的值
        cout << "socket: " << socket << endl;

        // 初始化启动信息结构体，并设置其大小
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        // 初始化进程信息结构体
        ZeroMemory(&pi, sizeof(pi));

        // 将命令行数据格式化为字符串并存储在lpHandle中
        wsprintf(lpHandle, TEXT("asyn_child.exe %d"), socket);

        // 创建一个新的进程
        if (!CreateProcess(NULL, lpHandle, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
            cout << "failed to create a child process." << endl;
        return;
    }
    void CreateThreadToHandleRequest(SOCKET socket)
    {
        DWORD threadID;
        HANDLE threadHandle;
        threadHandle = CreateThread(NULL, 0, RequestThread, &socket, 0, &threadID);
    }

    static ERROR_CODE HandleRequest(const string &msg, string &ss)
    {
        cout << msg << endl;
    }

    static DWORD WINAPI RequestThread(LPVOID args)
    {
        SOCKET socket = *(SOCKET *)args;
        char revData[255];
        int len;
        WSABUF buf1;
        buf1.buf = new char[1024];
        buf1.len = 1024;

        WSABUF *ppBuf = &buf1;
        DWORD dwRecvBytes = 0;
        DWORD dwFlags = 0;
        WSAOVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(WSAOVERLAPPED));
        overlapped.hEvent = WSACreateEvent();
        while (WSARecv(socket, ppBuf, 1, &dwRecvBytes, &dwFlags, &overlapped, NULL) != SOCKET_ERROR)
        {
            cout << ppBuf->buf << endl;
            string sendData = "hello world";
            if (len > 0)
            {
                revData[len] = 0x00;
                string msg(revData);
                HandleRequest(msg, sendData);
            }
            // cin >> sendData;
            // send(socket, sendData.c_str(), sendData.size(), 0);
            char *sendDa = (char *)sendData.c_str();
            WSABUF buf;
            buf.buf = sendDa;
            buf.len = sendData.size();

            WSABUF *pBuf = &buf;
            DWORD dwSendBytes = 0;
            DWORD dwFlags = 0;
            WSAOVERLAPPED overlapped;
            memset(&overlapped, 0, sizeof(WSAOVERLAPPED));
            overlapped.hEvent = WSACreateEvent();
            WSASend(socket, pBuf, 1, &dwSendBytes, dwFlags, &overlapped, NULL);
            // WSASend(socket, (LPWSABUF)&sendData, sendData.size(), NULL, 0, NULL, NULL);
        }
        cout << "request thread exit." << endl;
        closesocket(socket);
    }
};

int main()
{
    Server server(9988);
    return 0;
}