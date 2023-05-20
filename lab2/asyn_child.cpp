#include <winsock2.h>
#include <iostream>
#include <string>
#include <windows.h>
using namespace std;

int main(int argc, char *argv[])
{
    // 初始化WSA
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0)
    {
        return 0;
    }

    SOCKET socket;
    char revData[255];

    if (2 == argc)
    {
        socket = atoi(argv[1]);
        cout << "get the socket and handle in child process. " << socket << endl;
    }

    if (socket == INVALID_SOCKET)
    {
        cout << "invalid socket inherited from main process." << endl;
    }

    int ret;
    while ((ret = recv(socket, revData, 255, 0)) > 0)
    {
        revData[ret] = 0x0;
        cout << revData << endl;
        char sendData[255];
        cin >> sendData;
        send(socket, (const char*)sendData, strlen(sendData), 0);
    }

    WSACleanup();
    return 0;
}