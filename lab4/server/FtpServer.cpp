#include "FtpServer.hpp"
void FtpUtil::Init()
{
    string s = u8"╔══════════════════════════════════════════╗\n";
    s += u8"║                                          ║\n";
    s += u8"║         Welcome to Linexus's FTP         ║\n";
    s += u8"║                (Server)                  ║\n";
    s += u8"║                                          ║\n";
    s += u8"╚══════════════════════════════════════════╝";
    print_c(s, 3);
    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        WSACleanup();
        error("WSAStartup failed");
    }
    print_c("WSAStartup success", 2);
    // 获取主机名
    if (gethostname(hostname, HOSTNAME_LENGTH) == SOCKET_ERROR)
    {
        error("gethostname failed");
    }
    print_c("hostname: " + string(hostname), 14);
}

int FtpUtil::createSocket()
{
    socketDiscriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (socketDiscriptor == INVALID_SOCKET)
        error("socket failed", WSAGetLastError());
    else if (socketDiscriptor == SOCKET_ERROR)
        error("socket error", WSAGetLastError());
    else
        print_c("socket success", 2);
    bool opt = 1;
    int flag = setsockopt(socketDiscriptor, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(int));
    if (flag == SOCKET_ERROR)
    {
        print_c("setsockopt error" + to_string(WSAGetLastError()), 4);
    }

    return socketDiscriptor;
}

void FtpUtil::error(const char *msg, ...)
{
    string error = "error: ";
    error += msg;
    print_c(error, 4);
    exit(1);
}

bool FtpUtil::fileExists(const char *filename)
{
    ifstream ifile(filename);
    return ifile.is_open();
}

bool FtpUtil::readFileAndSend(const char *filename, SOCKET sock)
{
    // 传输包
    Package package; // 传输包
    package.footer = 0;

    print_c("Opening the file for reading ...", 14);
    ifstream ifile(filename, ios::in | ios::binary); // 以二进制方式打开文件
    // 判断文件是否打开成功
    if (!ifile)
    {
        print_c("Error opening the file to read :(", 4);
        return false;
    }
    // 读取文件并发送
    while (ifile)
    {
        ifile.read(package.buffer, BUFFER_SIZE);
        if (ifile.gcount() < BUFFER_SIZE)
            package.footer = ifile.gcount();
        send(sock, (char *)&package, sizeof(package), 0);
    }
    print_c("File sent successfully!", 2);
    ifile.close();
    return true;
}

bool FtpUtil::recvFileAndWrite(const char *filename, SOCKET sock)
{
    // 传输包
    Package package; // 传输包
    package.footer = 0;
    print_c("Opening the file for writing ...", 14);
    ofstream ofile(filename, ios::out | ios::binary); // 以二进制方式打开文件

    if (!ofile)
    {
        print_c("Error opening the file to write :(", 4);
        return false;
    }
    // 接收文件并写入
    while (true)
    {
        int test = recv(sock, (char *)&package, sizeof(package), 0);
        if (test == 0)
            break;
        if (package.footer == 0)
            ofile.write(package.buffer, BUFFER_SIZE);
        else
        {
            ofile.write(package.buffer, package.footer);
            break;
        }
    }
    print_c("File received successfully!", 2);
    ofile.close();
    return true;
}

bool FtpUtil::sendFileList(SOCKET sock)
{

    Package package; // 传输包
    package.footer = 0;
    print_c("Sending file list ...", 14);
    vector<string> files;
    string path = ".\\*.*";
    WIN32_FIND_DATAA fileData;
    // 打开当前路径
    HANDLE hFind = FindFirstFileA(path.c_str(), &fileData);
    ostringstream oss;
    if (hFind == INVALID_HANDLE_VALUE)
    {
        print_c("Error opening the file to write :(", 4);
        return false;
    }
    // 打印表头
    oss << setw(25) << left << "name" << setw(15) << right << "size" << setw(30) << right << "last revise" << endl;
    print_c(oss.str(), 5, true);
    files.push_back(oss.str());
    oss.str("");
    do
    {
        if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            readfileInfo(oss, ".\\" + string(fileData.cFileName));
            files.push_back(oss.str());
            // 打印出当前路径的所有文件的信息包括文件名、文件大小、文件最后修改时间
            print_c(oss.str(), 13, true);
            oss.str("");
        }

    } while (FindNextFileA(hFind, &fileData) != 0);
    // 关闭当前路径
    FindClose(hFind);
    // 发送文件列表
    for (int i = 0; i < files.size() - 1; i++)
    {
        memset(package.buffer, 0, BUFFER_SIZE);
        strcpy(package.buffer, files[i].c_str());
        package.footer = 1;
        send(sock, (char *)&package, sizeof(package), 0);
    }
    memset(package.buffer, 0, BUFFER_SIZE);
    strcpy(package.buffer, files[files.size() - 1].c_str());
    package.footer = 0; // 传输结束
    send(sock, (char *)&package, sizeof(package), 0);
    return true;
}

void FtpUtil::readfileInfo(ostringstream &oss, const string &path)
{
    WIN32_FIND_DATAA fileData;
    HANDLE hFind = FindFirstFileA(path.c_str(), &fileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        // 获取文件名
        string filename = fileData.cFileName;
        // 获取文件修改时间
        string fileTime;
        time_t modifiedTime = ((time_t)fileData.ftLastWriteTime.dwHighDateTime << 32) + fileData.ftLastWriteTime.dwLowDateTime;
        modifiedTime = static_cast<time_t>(modifiedTime / 10000000ULL - 11644473600ULL);
        struct tm localTime; // 定义一个 struct tm 类型的变量 localTime，用于存储本地时间
        localtime_s(&localTime, &modifiedTime);
        char timeString[20]; // 定义一个字符数组 timeString，用于存储格式化后的时间字符串
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &localTime);
        fileTime = timeString;
        // 获取文件大小
        auto fileSize = fileData.nFileSizeLow;
        string sizeString;
        if (fileSize >= 1e9)
        {
            sizeString = to_string(fileSize / 1e9);
            sizeString = setprecision(sizeString) + " GB";
        }
        else if (fileSize >= 1e6)
        {
            sizeString = to_string(fileSize / 1e6);
            sizeString = setprecision(sizeString) + " MB";
        }
        else if (fileSize >= 1e3)
        {
            sizeString = to_string(fileSize / 1e3);
            sizeString = setprecision(sizeString) + " KB";
        }
        else
            sizeString = to_string(fileSize) + " B";
        // 将文件名、文件大小、文件最后修改时间写入 oss
        oss << setw(25) << left << filename << setw(15) << right << sizeString << setw(30) << right << fileTime << endl;
        FindClose(hFind);
    }
}

string &FtpUtil::setprecision(string &str, int precision)
{
    size_t dotPos = str.find('.');
    if (dotPos != std::string::npos && dotPos + precision < str.length())
    {
        str.erase(dotPos + precision);
    }
    return str;
}

FtpServer::FtpServer()
{
    int debug = 0;
    // 加载网络编程库，打印前置信息
    FtpUtil::Init();

    // 创建套接字
    sock = createSocket();

    // 设置服务器地址
    memset(&ServerAddr, 0, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(LISTEN_PORT);
    ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 绑定套接字
    if (bind(sock, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {
        print_c("wrong code: " + to_string(WSAGetLastError()), 4);
    }
    else
        print_c("Socket Bound to port : " + to_string(LISTEN_PORT), 14);

    // 监听套接字
    debug = listen(sock, MAX_CLIENT_NUM);
    if (debug == SOCKET_ERROR)
        error("listen failed", WSAGetLastError());
    else
        print_c("Server Listen mode : " + to_string(debug), 14);
    print_c("Server is ready to accept connections!", 2);

    FD_ZERO(&clientSet);      // 设置套接字集，将监听套接字加入集合
    FD_SET(sock, &clientSet); // 监听套接字不需要加入set集合中
    maxfd = sock;             // 设置最大套接字描述符
}

FtpServer::~FtpServer()
{
    WSACleanup();
    print_c("Server is closed!", 14);
}

void FtpServer::recvNewJob()
{
    // 用于接收客户端的连接
    fd_set readfds;
    FD_ZERO(&readfds);
    readfds = clientSet;
    print_c("Waiting for new job ...", 14);
    // 用于接收客户端的连接
    int act = select(0, &readfds, NULL, NULL, NULL);
    if (act < 0)
        error("select failed", WSAGetLastError());
    // 如果有新的客户端连接
    if (FD_ISSET(sock, &readfds))
    {
        print_c("New client is coming!", 14);
        SOCKET sc = acceptClient();
        if (sc == SOCKET_ERROR)
        {
            print_c("Client connection failed!", 4);
            return;
        }
        FD_SET(sc, &clientSet);
    }
    // 循环遍历客户端套接字集合，查看是否有客户端发送文件
    for (auto it = clientSockets.begin(); it != clientSockets.end(); it++)
    {
        if (FD_ISSET(it->first, &readfds))
        {
            if (!getFile(it->first))
            {
                print_c("Client connection failed!", 4);
                return;
            }
            handleFile(it->second);
            break;
        }
    }
}

SOCKET FtpServer::acceptClient()
{
    // 如果客户端数量已满，拒绝连接
    if (clientSockets.size() > MAX_CLIENT_NUM)
    {
        print_c("Client number is full!", 4);
        return SOCKET_ERROR;
    }
    Client client;
    auto cli = make_shared<Client>(client);
    int size = sizeof(struct sockaddr);
    // 接收客户端连接
    SOCKET sc = accept(sock, (struct sockaddr *)&cli->addr, &size);
    if (sc != INVALID_SOCKET && sc != 0)
    {
        cli->sc = sc;
        // 将客户端套接字加入集合
        clientSockets.insert(pair<SOCKET, shared_ptr<Client>>(sc, cli));
        maxfd = maxfd > sc ? maxfd : sc;
        return sc;
    }
    return SOCKET_ERROR;
}

bool FtpServer::getFile(SOCKET sc)
{
    // 接收客户端发送的文件信息
    File file;
    dbug = recv(sc, (char *)&file, sizeof(File), 0);
    if (dbug < 0)
    {
        error("recv failed", WSAGetLastError());
        return false;
    }
    else if (dbug == 0) // 客户端断开连接，从集合中删除
    {
        disconnectClient(sc);
        return false;
    }
    // 打印客户端请求信息
    string s = "Client request to ";
    s += file.opId == OP_UP ? "upload" : (file.opId == OP_DOWN ? "download" : "list");
    s += " file: ";
    s += file.filename;
    print_c(s, 14);
    // 将客户端请求信息保存到客户端结构体中
    clientSockets[sc]->file.opId = file.opId;
    strcpy(clientSockets[sc]->file.filename, file.filename);
    return true;
}

bool FtpServer::handleFile(shared_ptr<Client> client)
{
    // 根据客户端请求信息，执行相应操作
    if (client->file.opId == OP_UP)
    {
        return up(client);
    }
    else if (client->file.opId == OP_DOWN)
    {
        return down(client);
    }
    else if (client->file.opId == OP_LIST)
    {
        return list(client);
    }
    return false;
}

bool FtpServer::down(shared_ptr<Client> client)
{
    // 发送客户端准备好接收文件的状态码
    print_c("Sending client ready state for download", 14);
    char rdyState[2] = "d"; // 状态码为d，表示服务器准备好接收文件
    // 发送状态码
    dbug = send(client->sc, rdyState, sizeof(rdyState), 0); // 发送状态码
    // 读取文件并发送
    bool is_send = readFileAndSend(client->file.filename, client->sc); // 读取文件并发送
    if (!is_send)
    {
        print_c("File sending failed", 4);
        return false;
    }
    return true;
}

bool FtpServer::up(shared_ptr<Client> client)
{
    print_c("Sending client ready state for upload", 14);
    char rdyState[2] = "u";                                 // 状态码为u，表示服务器准备好接收文件
    dbug = send(client->sc, rdyState, sizeof(rdyState), 0); // 发送状态码

    bool is_recv = recvFileAndWrite(client->file.filename, client->sc); // 接收文件并写入
    if (!is_recv)
    {
        print_c("File receiving failed", 4);
        return false;
    }
    return true;
}

bool FtpServer::list(shared_ptr<Client> client)
{
    print_c("Sending client ready state for list", 14);

    char rdyState[2] = "l";                                 // 状态码为l，表示服务器准备好发送文件列表
    dbug = send(client->sc, rdyState, sizeof(rdyState), 0); // 发送状态码

    bool is_send = sendFileList(client->sc); // 发送文件列表
    if (!is_send)
    {
        print_c("File list sending failed", 4);
        return false;
    }
    return false;
}

void FtpServer::disconnectClient(SOCKET sc)
{
    // 客户端断开连接，从集合中删除
    print_c("Client disconnected!", 14);
    closesocket(sc);
    FD_CLR(sc, &clientSet);
    clientSockets.erase(sc);
}

void FtpUtil::print_c(const string &msg, int color, bool isEndl)
{
    // 给控制台设置颜色
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
    string emoji = "";
    // 4: red, 14: yellow, 2: green, 13: purple
    switch (color)
    {
    case 4:
        emoji = u8"💀 "; // u8"🔴 ";
        break;
    case 14:
        emoji = u8"💡 "; // u8"🟡 ";
        break;
    case 2:
        emoji = u8"🔋 "; // u8"🟢 ";
        break;
    case 13:
        emoji = u8"📄 "; // u8"🟣 ";
        break;
    default:
        break;
    }
    // 判断是否换行
    if (!isEndl)
        cout << emoji << msg << endl;
    else
        cout << emoji << msg;
    SetConsoleTextAttribute(hConsole, 15);
}

int main()
{
    int whileFlag = 1;
    // 创建FTP服务器对象
    unique_ptr<FtpServer> ftpServer(new FtpServer());
    // 初始化服务器
    while (whileFlag)
    {
        ftpServer->recvNewJob();
    }
    return 0;
}
