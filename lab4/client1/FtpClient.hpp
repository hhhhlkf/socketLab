#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <cstring>

using namespace std;

#define HOSTNAME_LENGTH 30
#define FILENAME_LENGTN 30
#define BUFFER_SIZE 1024

#define CONNECT_ON_PORT 11451

enum OP_ID
{
    OP_LIST = 0, // 列出文件
    OP_UP = 1,   // 上传
    OP_DOWN = 2, // 下载
};

struct Package // 传输包
{
    int header;
    char buffer[BUFFER_SIZE];
    int footer;
};

struct File // 文件
{
    char filename[FILENAME_LENGTN]; // 文件名
    int opId;                       // 操作ID
};

class FtpUtil // FTP工具类
{
private:
    int socketDiscriptor;             // 套接字描述符
    struct sockaddr_in serverAddress; // 服务器地址
    unsigned short port;              // 端口号
    WSADATA wsaData;                  // WSA数据
    char hostname[HOSTNAME_LENGTH];   // 主机名

public:
    void Init();                                                    // 初始化
    int createSocket();                                             // 创建套接字
    void error(const char *msg, ...);                               // 错误处理
    bool fileExists(const char *filename);                          // 判断文件是否存在
    bool readFileAndSend(const char *filename, SOCKET sock);        // 读取文件并发送
    bool recvFileAndWrite(const char *filename, SOCKET sock);       // 接收文件并存入FTP服务器中
    unsigned long ResolveName(char name[]);                         // 解析主机名
    void print_c(const string &msg, int color, bool isEnd = false); // 打印信息
};

class FtpClient : public FtpUtil // FTP客户端
{
    struct sockaddr_in ServerAddr;        // 服务器地址
    char remoteHostname[HOSTNAME_LENGTH]; // 远程主机名
    int sock;                             // 套接字描述符
    int debug;                            // 调试模式

public:
    FtpClient();
    ~FtpClient();

    bool run();         // 运行
    bool down(File &f); // 下载
    bool up(File &f);   // 上传
    bool list();        // 列出文件
    bool quit();        // 退出
};