#include "FtpClient.hpp"

void FtpUtil::Init()
{
    string s = u8"╔══════════════════════════════════════════╗\n";
    s += u8"║                                          ║\n";
    s += u8"║         Welcome to Linexus's FTP         ║\n";
    s += u8"║                (Client)                  ║\n";
    s += u8"║                                          ║\n";
    s += u8"╚══════════════════════════════════════════╝";
    print_c(s, 5);
    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        WSACleanup();
        error("WSAStartup failed");
    }
    print_c("WSAStartup success", 11);
    // 获取主机名
    if (gethostname(hostname, HOSTNAME_LENGTH) == SOCKET_ERROR)
    {
        error("gethostname failed");
    }
    print_c("hostname: " + string(hostname), 14);
}

int FtpUtil::createSocket()
{
    // 创建套接字
    socketDiscriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // 检查套接字是否创建成功
    if (socketDiscriptor == INVALID_SOCKET)
        error("socket failed", WSAGetLastError());
    else if (socketDiscriptor == SOCKET_ERROR)
        error("socket error", WSAGetLastError());
    else
        print_c("socket success", 11);
    bool opt = 1;
    return socketDiscriptor;
}

void FtpUtil::error(const char *msg, ...)
{
    string error = "error: ";
    error += msg;
    print_c(error, 12);
    exit(1);
}

bool FtpUtil::fileExists(const char *filename)
{
    // 检查文件是否存在
    ifstream ifile(filename);
    return ifile.is_open();
}

unsigned long FtpUtil::ResolveName(char name[])
{
    // 获取主机信息
    struct hostent *host; /* Structure containing host information */

    if ((host = gethostbyname(name)) == NULL)
        error("gethostbyname() failed");

    // 返回主机地址
    return *((unsigned long *)host->h_addr_list[0]);
}

bool FtpUtil::readFileAndSend(const char *filename, SOCKET sock)
{
    Package package; // 传输包
    package.footer = 0;

    print_c("Opening the file for reading ...", 14);
    ifstream ifile(filename, ios::in | ios::binary); // 以二进制方式打开文件

    if (!ifile)
    {
        print_c("Error opening the file to read :(", 12);
        return false;
    }
    // 循环读取文件并发送
    while (ifile)
    {
        ifile.read(package.buffer, BUFFER_SIZE);
        // 如果读取的字节数小于缓冲区大小，则说明是最后一个包
        if (ifile.gcount() < BUFFER_SIZE)
            package.footer = ifile.gcount();
        send(sock, (char *)&package, sizeof(package), 0);
    }
    print_c("File sent successfully!", 11);
    ifile.close();
    return true;
}

bool FtpUtil::recvFileAndWrite(const char *filename, SOCKET sock)
{
    Package package; // 传输包
    package.footer = 0;

    print_c("Opening the file for writing ...", 14);
    ofstream ofile(filename, ios::out | ios::binary); // 以二进制方式打开文件

    if (!ofile)
    {
        print_c("Error opening the file to write :(", 12);
        return false;
    }
    // 循环接收文件并写入
    while (true)
    {
        int test = recv(sock, (char *)&package, sizeof(package), 0);
        if (test == 0)
            break;
        // 如果是最后一个包，则写入剩余的字节
        if (package.footer == 0)
            ofile.write(package.buffer, BUFFER_SIZE);
        else
        {
            ofile.write(package.buffer, package.footer);
            break;
        }
    }
    print_c("File received successfully!", 11);
    ofile.close();
    return true;
}

FtpClient::FtpClient()
{
    FtpUtil::Init();

    // 创建套接字
    sock = FtpUtil::createSocket();
    print_c("Socket created", 14);
    print_c("Please enter the hostname to connect to: ", 14, true);
    cin >> remoteHostname;

    // 获取远程主机地址
    memset(&ServerAddr, 0, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(CONNECT_ON_PORT);
    ServerAddr.sin_addr.s_addr = FtpUtil::ResolveName(remoteHostname);

    if (connect(sock, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {
        error("connect failed", WSAGetLastError());
    }
    print_c("Connected to the server successfully", 11);
}

FtpClient::~FtpClient()
{
    closesocket(sock);
    WSACleanup();
}

bool FtpClient::run()
{
    File file;
    bool running = true;
    while (running)
    {
        // 询问用户需要执行的操作
        print_c("Would you like to LIST(0), UPLOAD (1) or DOWNLOAD (2) or EXIT (else)?", 14);
        print_c("ftp> ", 5, true);
        cin >> file.opId;
        // 如果输入的不是上传指令或下载指令，则退出
        if (file.opId != OP_UP && file.opId != OP_DOWN && file.opId != OP_LIST)
        {
            print_c("Bye!", 14);
            quit();
            return true;
        }
        // 如果是上传或下载指令，则询问用户需要上传或下载的文件名
        strcpy(file.filename, "all files");
        if (file.opId != OP_LIST)
        {
            memset(file.filename, 0, sizeof(file.filename));
            print_c("Please enter the filename: ", 14);
            print_c("ftp> ", 5, true);
            cin >> file.filename;
        }
        // 给服务器发送文件名
        print_c("Sending the filename to the server ...", 14);
        print_c("file's id: " + to_string(file.opId), 14);
        debug = send(sock, (char *)&file, sizeof(file), 0);
        if (debug == SOCKET_ERROR)
            error("send failed", WSAGetLastError());
        else
            print_c("Filename sent successfully!", 11);

        // 接收服务器的响应
        print_c("Waiting for the server's response ...", 14);
        char rdyState[2];
        debug = recv(sock, rdyState, sizeof(rdyState), 0);
        if (debug == SOCKET_ERROR)
            error("recv failed", WSAGetLastError());
        else
            print_c("Server's response :" + string(rdyState), 11);
        // 根据用户的选择执行相应的操作
        switch (file.opId)
        {
        case OP_UP:
            up(file);
            break;
        case OP_DOWN:
            down(file);
            break;
        case OP_LIST:
            list();
            break;
        default:
            quit();
            return true;
        }
        // 询问用户是否继续操作
        print_c("Would you like to operate again? (y/n)", 14);
        print_c("ftp> ", 5, true);
        cin >> rdyState;
        if (strcmp(rdyState, "y") != 0)
            running = false;
        if (strcmp(rdyState, "n") == 0)
            quit();
    }
    return true;
}

bool FtpClient::down(File &f)
{
    // 接收文件
    print_c("Downloading the file ...", 14);
    if (!FtpUtil::recvFileAndWrite(f.filename, sock))
    {
        print_c("Error receiving the file :(", 12);
        return false;
    }
    return true;
}

bool FtpClient::up(File &f)
{
    // 上传文件
    print_c("Uploading the file ...", 14);
    if (!FtpUtil::fileExists(f.filename))
    {
        print_c("File does not exist :(", 12);
        return false;
    }
    if (!FtpUtil::readFileAndSend(f.filename, sock))
    {
        print_c("Error sending the file :(", 12);
        return false;
    }
    return true;
}

bool FtpClient::list()
{
    // 接收文件列表并打印
    Package p;
    p.footer = 0;
    print_c("Receiving the file list ...", 14);
    do
    {
        debug = recv(sock, (char *)&p, sizeof(p), 0);
        print_c(p.buffer, 13, true);
        if (debug == SOCKET_ERROR)
            error("recv failed", WSAGetLastError());
        if (p.footer == 0)
            break;
    } while (1);
    return true;
}

bool FtpClient::quit()
{
    // 退出
    print_c("Quitting ...", 14);
    if (shutdown(sock, 2) == SOCKET_ERROR)
    {
        print_c("shutdown failed: " + WSAGetLastError(), 12);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    closesocket(sock);
    WSACleanup();
    return true;
}

void FtpUtil::print_c(const string &msg, int color, bool isEndl)
{
    // 打印带颜色的信息
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
    string emoji = "";
    // 12: red, 14: yellow, 11: green, 13: blue
    switch (color)
    {
    case 12:
        emoji = u8"💀 "; // u8"🔴 ";
        break;
    case 14:
        emoji = u8"💡 "; // u8"🟡 ";
        break;
    case 11:
        emoji = u8"🔋 "; // u8"🟢 ";
        break;
    case 13:
        emoji = u8"📄 "; // u8"🔵 ";
        break;
    default:
        break;
    }
    // 判断是否需要换行
    if (!isEndl)
        cout << emoji << msg << endl;
    else
        cout << emoji << msg;
    SetConsoleTextAttribute(hConsole, 15);
}

int main(int argc, char const *argv[])
{
    // 初始化
    FtpClient *client = new FtpClient();
    // 连接服务器
    client->run();

    return 0;
}