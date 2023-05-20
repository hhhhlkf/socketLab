// 一个简单的raw socket示例，能够解析TCP包和IP包，且能在windows上运行，使用C++语言
// 参考：https://docs.microsoft.com/en-us/windows/win32/winsock/using-raw-sockets
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <conio.h>
#define IO_RCVALL _WSAIOW(IOC_VENDOR, 1)
#define BUFFER_SIZE 65535
using namespace std;
typedef pair<int, BYTE> pii;
char localName[256];       // 本地机器名
DWORD dwBufferLen[10];     // 获取主机名
DWORD dwBufferInLen = 1;   // 指向主机信息的指针
DWORD dwBytesReturned = 0; // 通过主机名获取本地IP地址
char buffer[BUFFER_SIZE];
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#define MAX_PACKET_SIZE 65536

// 一个简单的IP头结构体，只包含必要的字段
struct ip_header
{
    u_char version_ihl; // 版本和首部长度
    u_char tos;         // 服务类型
    u_short tot_len;    // 总长度
    u_short id;         // 标识符
    u_short frag_off;   // 分片偏移
    u_char ttl;         // 生存时间
    u_char protocol;    // 协议类型
    u_short checksum;   // 校验和
    u_long saddr;       // 源地址
    u_long daddr;       // 目的地址
};

// 一个简单的TCP头结构体，只包含必要的字段
struct tcp_header
{
    u_short source_port; // 源端口号
    u_short dest_port;   // 目的端口号
    u_long seq_number;   // 序列号
    u_long ack_number;   // 确认号
    u_char data_offset;  // 数据偏移
    u_char flags;        // 标志位
    u_short window;      // 窗口大小
    u_short checksum;    // 校验和
    u_short urgent_ptr;  // 紧急指针
};

// 一个函数，用于打印IP头的信息
void print_ip_header(struct ip_header *ip)
{
    printf("Version: %d\n", ip->version_ihl >> 4);
    printf("Header length: %d\n", (ip->version_ihl & 0x0F) * 4);
    printf("Type of service: %d\n", ip->tos);
    printf("Total length: %d\n", ntohs(ip->tot_len));
    printf("Identification: %d\n", ntohs(ip->id));
    printf("Fragment offset: %d\n", ntohs(ip->frag_off));
    printf("Time to live: %d\n", ip->ttl);
    printf("Protocol: %d\n", ip->protocol);
    printf("Checksum: %04x\n", ntohs(ip->checksum));
    printf("Source address: %s\n", inet_ntoa(*(struct in_addr *)&ip->saddr));
    printf("Destination address: %s\n", inet_ntoa(*(struct in_addr *)&ip->daddr));
}

// 一个函数，用于打印TCP头的信息
void print_tcp_header(struct tcp_header *tcp)
{
    printf("Source port: %d\n", ntohs(tcp->source_port));
    printf("Destination port: %d\n", ntohs(tcp->dest_port));
    printf("Sequence number: %u\n", ntohl(tcp->seq_number));
    printf("Acknowledgement number: %u\n", ntohl(tcp->ack_number));
    printf("Data offset: %d\n", (tcp->data_offset >> 4) * 4);
    printf("Flags: %02x\n", tcp->flags);
    printf("Window: %d\n", ntohs(tcp->window));
    printf("Checksum: %04x\n", ntohs(tcp->checksum));
    printf("Urgent pointer: %d\n", ntohs(tcp->urgent_ptr));
}

int main()
{
    WSADATA wsaData;
    SOCKET raw_sock;
    char buffer[MAX_PACKET_SIZE];
    int bytes_received;
    struct ip_header *ip;
    struct tcp_header *tcp;
    // 初始化Winsock库
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return -1;
    }
    // 创建一个原始套接字，用于接收IP层的数据包
    raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
    if (raw_sock == INVALID_SOCKET)
    {
        printf("socket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }
    // 绑定套接字到本地地址，这里假设是127.0.0.1
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    local_addr.sin_port = htons(8899); // 设定一个端口。不能是已固定的端口
    if (bind(raw_sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(raw_sock);
        WSACleanup();
        return -1;
    }
    int flag = WSAIoctl(raw_sock, IO_RCVALL, &dwBufferInLen, sizeof(dwBufferInLen), dwBufferLen,
                        sizeof(dwBufferLen), &dwBytesReturned, NULL, NULL);
    if (flag < 0)
    {
        printf("WSAIoctl failed with error: %d\n", WSAGetLastError());
        closesocket(raw_sock);
        WSACleanup();
        return -1;
    }

    // 循环接收数据包，并解析IP头和TCP头部信息
    int i = 5;
    while (true)
    {
        bytes_received = recv(raw_sock, buffer, MAX_PACKET_SIZE, 0);
        if (bytes_received == SOCKET_ERROR)
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            break;
        }
        else if (bytes_received > 0)
        {
            ip = (struct ip_header *)buffer; // IP头部在缓冲区的开始位置
            if (ip->protocol == IPPROTO_TCP)
            {
                tcp = (struct tcp_header *)(buffer + (ip->version_ihl & 0x0F) * 4); // 如果是TCP协议的数据包
                                                                                    // TCP头部在IP头部之后，注意要根据IP首部长度计算偏移量
            }
            if (strcmp(inet_ntoa(*(struct in_addr *)&ip->daddr), "127.0.0.1") != 0 ||
                ntohs(tcp->source_port) != 9988)
                continue;
            cout << "--------------------------IP首部--------------------------\n";
            print_ip_header(ip); // 打印IP头部信息
            if (ip->protocol == IPPROTO_TCP)
            {
                cout << "--------------------------TCP首部--------------------------\n";
                print_tcp_header(tcp); // 打印TCP头部信息
            }
            char *data;
            int data_len;
            data = buffer + (tcp->data_offset >> 4) * 4;             // 数据部分在TCP头部之后，注意要根据数据偏移计算偏移量
            data_len = bytes_received - (tcp->data_offset >> 4) * 4; // 数据部分的长度等于收到的字节数减去TCP头部的长度
            cout << "--------------------------数据部分--------------------------\n";
            printf("Data: ");
            for (int i = 0; i < data_len; i++)
            {
                printf("%c", data[i]); // 打印数据的内容
            }
            printf("\n");
        }
        else
        {
            break; // 没有收到数据包，退出循环
        }
    }

    // 关闭套接字和Winsock库
    closesocket(raw_sock);
    WSACleanup();
}
