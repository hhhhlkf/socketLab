#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <cstring>
#include <vector>
#include <sstream>
#include <ctime>
#include <iomanip>
using namespace std;
// FTP工具类
#define HOSTNAME_LENGTH 30
#define FILENAME_LENGTN 30
#define BUFFER_SIZE 1024

// FTPServer宏
#define LISTEN_PORT 11451
#define MAX_CLIENT_NUM 5

#define CLIENT_DISCONN 0 // 客户端断开连接
#define CLIENT_CONN 1    // 客户端连接

enum OP_ID
{
    OP_LIST = 0,
    OP_UP = 1,
    OP_DOWN = 2,
};

struct Package // 传输包
{
    int header;               // 0:文件名 1:文件内容
    char buffer[BUFFER_SIZE]; // 缓冲区
    int footer;               // 0:文件内容 1:文件名
};

struct File // 文件
{
    char filename[FILENAME_LENGTN]; // 文件名
    int opId;                       // 操作ID
};

struct Client // 客户端
{
    sockaddr_in addr; // 客户端地址
    File file;        // 文件
    SOCKET sc;        // 套接字
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
    void Init();                                              // 初始化
    int createSocket();                                       // 创建套接字
    void error(const char *msg, ...);                         // 错误处理
    bool fileExists(const char *filename);                    // 判断文件是否存在
    bool readFileAndSend(const char *filename, SOCKET sock);  // 读取文件并发送
    bool recvFileAndWrite(const char *filename, SOCKET sock); // 接收文件并存入FTP服务器中
    bool sendFileList(SOCKET sock);
    void readfileInfo(ostringstream &oss, const string &path);
    string& setprecision(string &str, int precision  = 3);
    void print_c(const string &msg, int color, bool flag = false);          // 打印信息
};

class FtpServer : public FtpUtil // FTP服务器
{
private:
    struct sockaddr_in ServerAddr;                           // 服务器地址
    char serverName[HOSTNAME_LENGTH];                        // 服务器名称
    int sock;                                                // 套接字
    fd_set clientSet;                                        // 客户端集合
    size_t maxfd;                                            // 客户端最大套接字，与客户端集合配合使用
    unordered_map<SOCKET, shared_ptr<Client>> clientSockets; // 客户端套接字集合
public:
    int dbug = 0;                               // 调试信息
    FtpServer();                                // 构造函数
    ~FtpServer();                               // 析构函数
    void recvNewJob();                          // 接收新任务，包含链接客户端和接收文件
    SOCKET acceptClient();                      // 接收客户端
    bool getFile(SOCKET sc);                    // 获取文件的信息和操作
    bool handleFile(shared_ptr<Client> client); // 处理文件
    bool down(shared_ptr<Client> client);       // 下载文件
    bool up(shared_ptr<Client> client);         // 上传文件
    void disconnectClient(SOCKET sc);           // 断开客户端连接
    bool list(shared_ptr<Client> client);                // 获取文件列表
};
