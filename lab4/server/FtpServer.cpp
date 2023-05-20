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
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        WSACleanup();
        error("WSAStartup failed");
    }
    print_c("WSAStartup success", 2);
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
    // cout << "socket success" << endl;
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
    // perror(NULL);
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
    Package package; // 传输包
    package.footer = 0;

    // cout << "Opening the file for reading ..." << endl;
    print_c("Opening the file for reading ...", 14);
    ifstream ifile(filename, ios::in | ios::binary); // 以二进制方式打开文件

    if (!ifile)
    {
        print_c("Error opening the file to read :(", 4);
        // cout << "Error opening the file to read :(" << endl;
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
    // cout << "File sent successfully!" << endl;
    ifile.close();
    return true;
}

bool FtpUtil::recvFileAndWrite(const char *filename, SOCKET sock)
{
    Package package; // 传输包
    package.footer = 0;
    print_c("Opening the file for writing ...", 14);
    // cout << "Opening the file for writing ..." << endl;
    ofstream ofile(filename, ios::out | ios::binary); // 以二进制方式打开文件

    if (!ofile)
    {
        print_c("Error opening the file to write :(", 4);
        // cout << "Error opening the file to write :(" << endl;
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
            // cout << "last packet sent of size: " << p.footer << endl;
            break;
        }
    }
    print_c("File received successfully!", 2);
    // cout << "File received successfully!" << endl;
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
    HANDLE hFind = FindFirstFileA(path.c_str(), &fileData);
    ostringstream oss;
    if (hFind == INVALID_HANDLE_VALUE)
    {
        print_c("Error opening the file to write :(", 4);
        return false;
    }
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
            print_c(oss.str(), 13, true);
            oss.str("");
        }

    } while (FindNextFileA(hFind, &fileData) != 0);
    // 打印出当前路径的所有文件
    FindClose(hFind);
    for (int i = 0; i < files.size() - 1; i++)
    {
        memset(package.buffer, 0, BUFFER_SIZE);
        strcpy(package.buffer, files[i].c_str());
        package.footer = 1;
        send(sock, (char *)&package, sizeof(package), 0);
    }
    memset(package.buffer, 0, BUFFER_SIZE);
    strcpy(package.buffer, files[files.size() - 1].c_str());
    package.footer = 0;
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
    // cout << "Socket Bound to port : " << LISTEN_PORT << endl;

    // 监听套接字
    debug = listen(sock, MAX_CLIENT_NUM);
    if (debug == SOCKET_ERROR)
        error("listen failed", WSAGetLastError());
    else
        print_c("Server Listen mode : " + to_string(debug), 14);
    // cout << "Server Listen mode : " << debug << endl;
    print_c("Server is ready to accept connections!", 2);
    // cout << "Server is ready to accept connections!" << endl;

    FD_ZERO(&clientSet);      // 设置套接字集，将监听套接字加入集合
    FD_SET(sock, &clientSet); // 监听套接字不需要加入set集合中
    maxfd = sock;             // 设置最大套接字描述符
}

FtpServer::~FtpServer()
{
    WSACleanup();
    print_c("Server is closed!", 14);
    // cout << "Server is closed!" << endl;
}

void FtpServer::recvNewJob()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    readfds = clientSet;
    print_c("Waiting for new job ...", 14);
    // cout << "Waiting for new job ..." << endl;
    int act = select(0, &readfds, NULL, NULL, NULL);
    if (act < 0)
        error("select failed", WSAGetLastError());
    if (FD_ISSET(sock, &readfds))
    {
        print_c("New client is coming!", 14);
        // cout << "New client is coming!" << endl;
        SOCKET sc = acceptClient();
        if (sc == SOCKET_ERROR)
        {
            print_c("Client connection failed!", 4);
            // cout << "Client connection failed!" << endl;
            return;
        }
        FD_SET(sc, &clientSet);
    }
    for (auto it = clientSockets.begin(); it != clientSockets.end(); it++)
    {
        if (FD_ISSET(it->first, &readfds))
        {
            if (!getFile(it->first))
            {
                print_c("Client connection failed!", 4);
                // cout << "Client connection failed!" << endl;
                return;
            }
            handleFile(it->second);
            break;
        }
    }
}

SOCKET FtpServer::acceptClient()
{
    if (clientSockets.size() > MAX_CLIENT_NUM)
    {
        print_c("Client number is full!", 4);
        // cout << "Client number is full!" << endl;
        return SOCKET_ERROR;
    }
    Client client;
    auto cli = make_shared<Client>(client);
    int size = sizeof(struct sockaddr);
    SOCKET sc = accept(sock, (struct sockaddr *)&cli->addr, &size);
    if (sc != INVALID_SOCKET && sc != 0)
    {
        cli->sc = sc;
        clientSockets.insert(pair<SOCKET, shared_ptr<Client>>(sc, cli));
        // FD_SET(sc, &clientSet);
        maxfd = maxfd > sc ? maxfd : sc;
        return sc;
    }
    return SOCKET_ERROR;
}

bool FtpServer::getFile(SOCKET sc)
{
    File file;
    dbug = recv(sc, (char *)&file, sizeof(File), 0);
    if (dbug < 0)
    {
        error("recv failed", WSAGetLastError());
        return false;
    }
    else if (dbug == 0)
    {
        disconnectClient(sc);
        return false;
    }
    string s = "Client request to ";
    s += file.opId == OP_UP ? "upload" : (file.opId == OP_DOWN ? "download" : "list");
    s += " file: ";
    s += file.filename;
    print_c(s, 14);

    clientSockets[sc]->file.opId = file.opId;
    strcpy(clientSockets[sc]->file.filename, file.filename);
    return true;
}

bool FtpServer::handleFile(shared_ptr<Client> client)
{
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
    print_c("Sending client ready state for download", 14);
    // cout << "Sending client ready state for download" << endl;
    char rdyState[2] = "d"; // 状态码为d，表示服务器准备好接收文件

    dbug = send(client->sc, rdyState, sizeof(rdyState), 0); // 发送状态码

    bool is_send = readFileAndSend(client->file.filename, client->sc); // 读取文件并发送
    if (!is_send)
    {
        print_c("File sending failed", 4);
        // cout << "File sending failed" << endl;
        return false;
    }
    // dbug = recv(client->sc, rdyState, sizeof(rdyState), 0); // 接收客户端的状态码
    // if (dbug < 0)
    //     cout << "Client disconnected" << endl;
    return true;
}

bool FtpServer::up(shared_ptr<Client> client)
{
    print_c("Sending client ready state for upload", 14);
    // cout << "Sending client ready state for upload" << endl;
    char rdyState[2] = "u";                                 // 状态码为u，表示服务器准备好接收文件
    dbug = send(client->sc, rdyState, sizeof(rdyState), 0); // 发送状态码

    bool is_recv = recvFileAndWrite(client->file.filename, client->sc); // 接收文件并写入
    if (!is_recv)
    {
        print_c("File receiving failed", 4);
        // cout << "File receiving failed" << endl;
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
        // cout << "File list sending failed" << endl;
        return false;
    }
    return false;
}

void FtpServer::disconnectClient(SOCKET sc)
{
    print_c("Client disconnected!", 14);
    // cout << "Client disconnected!" << endl;
    closesocket(sc);
    FD_CLR(sc, &clientSet);
    clientSockets.erase(sc);
}

void FtpUtil::print_c(const string &msg, int color, bool isEndl)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
    string emoji = "";
    switch (color)
    {
    case 4:
        emoji = u8"💀 ";
        break;
    case 14:
        emoji = u8"💡 ";
        break;
    case 2:
        emoji = u8"🔋 ";
        break;
    case 13:
        emoji = u8"📄 ";
        break;
    default:
        break;
    }
    if (!isEndl)
        cout << emoji << msg << endl;
    else
        cout << emoji << msg;
    SetConsoleTextAttribute(hConsole, 15);
}

int main()
{
    int whileFlag = 1;
    unique_ptr<FtpServer> ftpServer(new FtpServer());
    while (whileFlag)
    {
        ftpServer->recvNewJob();
    }
    return 0;
}
