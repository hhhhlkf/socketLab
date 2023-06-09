#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char *argv[])
{
    WORD socketVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(socketVersion, &wsaData) != 0)
    {
        return 0;
    }
    SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in sin;                                   // 服务器地址
    sin.sin_family = AF_INET;                          // 指定地址族
    sin.sin_port = htons(8888);                        // 指定端口
    sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 指定IP地址
    int len = sizeof(sin);                             // 服务器地址长度
    int num = 5;
    while (num--)
    {
        const char *sendData = "the package from Client.\n";
        sendto(sclient, sendData, strlen(sendData), 0, (sockaddr *)&sin, len);

        char recvData[255];
        int ret = recvfrom(sclient, recvData, 255, 0, (sockaddr *)&sin, &len);
        if (ret > 0)
        {
            recvData[ret] = 0x00;
            printf(recvData);
        }
    }
    closesocket(sclient);
    WSACleanup();
    return 0;
}