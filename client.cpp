#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <string>
#include <sstream>
#include <algorithm>
#include <deque>
#include <vector>
#include <map>
#include <utility>
#include <thread>
#include <mutex>
#include "npinc.hpp"
#include "nptype.hpp"
#include "nputility.hpp"
#include "message.hpp"
#include "ThreadUtil.hpp"
#include "ClientUtility.hpp"

// client variables
winsize ws;
int p2pPort;
ClientUtility clientUtility;
// client init functions
ConnectInfo parseArgument(const int& argc, const char**& argv);
void fileListWatcher();
std::set<std::string> getLocalFileList;
// client main functions
void clientFunc(const ConnectData& server);
void clientRecv(const int fd);
// p2p server for file transfer or chat
int p2pServerInit();
void p2pServerAccept(const int listenfd);
void p2pServerFunc(const int fd);

int main(int argc, const char** argv) {
    setbuf(stdout, NULL);
    parseArgument(argc, argv);
    lb::setLogEnabled(false);
    lb::threadManageInit();
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    mkdir("Client", 0777);
    ConnectInfo connectInfo = parseArgument(argc, argv);
    ConnectData server = connectTo(connectInfo);
    if (server.fd < 0) {
        lb::joinAll();
        return EXIT_FAILURE;
    }
    p2pPort = p2pServerInit();
    clientFunc(server);
    lb::joinAll();
    return EXIT_SUCCESS;
}

ConnectInfo parseArgument(const int& argc, const char**& argv) {
    if (argc != 3) {
        fprintf(stderr, "usage %s <server address> <port>\n", argv[0]);
        lb::joinAll();
        exit(EXIT_FAILURE);
    }
    std::string address;
    int port;
    address = argv[1];
    if (sscanf(argv[2], "%d", &port) != 1) {
        fprintf(stderr, "%s: not a valid port number\n", argv[2]);
        lb::joinAll();
        exit(EXIT_FAILURE);
    }
    return ConnectInfo(address, port);
}

void clientFunc(const ConnectData& server) {
    clientUtility.init(server.fd, p2pPort, ws.ws_row, ws.ws_col);
    char buffer[MAXN];
    clientUtility.setStage(NPStage::WELCOME);
    clientUtility.printMessage("Welcome!");
    while (true) {
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(fileno(stdin), &fdset);
        timeval tv = tv1s;
        int nready = select(fileno(stdin) + 1, &fdset, NULL, NULL, &tv);
        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "select: %s\n", strerror(errno));
            break;
        }
        if (FD_ISSET(fileno(stdin), &fdset)) {
            if (fgets(buffer, MAXN, stdin) == NULL) {
                clientUtility.printPrevious();
                continue;
            }
            trimNewLine(buffer);
            toUpperString(buffer);
            std::string command(buffer);
            if (command == "") {
                clientUtility.printPrevious();
                continue;
            }
            if (command == "Q" || command == "QUIT") {
                clientUtility.logout();
                printf("\n");
                break;
            }
            switch (static_cast<int>(clientUtility.getStage())) {
                case 0: // WELCOME
                    if (command == "R") {
                        clientUtility.newAccount();
                    }
                    else if (command == "L") {
                        clientUtility.login();
                    }
                    else {
                        clientUtility.printMessage("Invalid command", true);
                    }
                    break;
                case 1: // MAIN
                    if (command == "L") {
                        clientUtility.logout();
                    }
                    else if (command == "DA") {
                        clientUtility.deleteAccount();
                    }
                    else if (command == "SU") {
                        clientUtility.showUser();
                    }
                    else if (command == "SF") {
                        clientUtility.showFileList();
                    }
                    else if (command == "SE") {
                        clientUtility.updateFileList();
                    }
                    else if (command == "R") {
                        clientUtility.getFileList();
                    }
                    else if (command == "U") {
                        clientUtility.upload();
                    }
                    else if (command == "D") {
                        clientUtility.download();
                    }
                    else if (command == "C") {
                        clientUtility.chat();
                    }
                    else if (command == "LS") {
                        clientUtility.ls();
                    }
                    else {
                        clientUtility.printMessage("Invalid command", true);
                    }
                default:
                    break;
            }
        }
        bool isServerAlive = clientUtility.checkConnection();
        if (!isServerAlive) {
            clientUtility.printMessage(msgDISCONNECTED, true);
            putc('\n', stdout);
            break;
        }
        clientUtility.updateDir();
    }
    close(server.fd);
}

int p2pServerInit() {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::minstd_rand randomGenerator(seed);
    int port;
    int serverfd;
    while (true) {
        port = randomGenerator() % 1000 + 50000;
        if ((serverfd = newServer(port)) >= 0) {
            break;
        }
    }
    lb::pushThread(std::thread(p2pServerAccept, serverfd));
    return port;
}

void p2pServerAccept(const int listenfd) {
    while (lb::isValid()) {
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(listenfd, &fdset);
        timeval tv = tv1s;
        int nready = select(listenfd + 1, &fdset, NULL, NULL, &tv);
        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "select: %s\n", strerror(errno));
            break;
        }
        if (FD_ISSET(listenfd, &fdset)) {
            ConnectData client = acceptConnection(listenfd);
            lb::pushThread(std::thread(p2pServerFunc, client.fd));
        }
    }
    close(listenfd);
    lb::finishThread();
}

void p2pServerFunc(const int fd) {
    char buffer[MAXN];
    while (lb::isValid()) {
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);
        timeval tv = tv200ms;
        int nready = select(fd + 1, &fdset, NULL, NULL, &tv);
        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "select: %s\n", strerror(errno));
            break;
        }
        if (FD_ISSET(fd, &fdset)) {
            if (tcpRead(fd, buffer, MAXN) <= 0) {
                break;
            }
            std::string command(buffer);
            if (command.find(msgCHECKCONNECT) == 0u) {
                tcpWrite(fd, msgCHECKCONNECT);
            }
            else if (command.find(msgMESSAGE) == 0u) {
                char account[MAXN];
                sscanf(command.c_str() + msgMESSAGE.length(), "%s", account);
                unsigned offset = msgMESSAGE.length() + 1 + std::string(account).length() + 1;
                std::string content(command.c_str() + offset);
                clientUtility.pushMessage(account, content);
            }
            else if (command.find(msgFILEWRITE) == 0u) {
                char filename[MAXN];
                std::string filepath;
                unsigned long fileSize;
                sscanf(command.c_str() + msgFILEWRITE.length(), "%s%lu", filename, &fileSize);
                filepath = std::string("./Client/") + filename;
                FILE* fp = fopen(filepath.c_str(), "wb");
                if (!fp) {
                    std::string errMsg = msgFAIL + " Server: " +filename + " " + strerror(errno);
                    tcpWrite(fd, errMsg);
                    break;
                }
                tcpWrite(fd, msgSUCCESS);
                unsigned long byteRead = 0;
                while (byteRead < fileSize) {
                    char content[MAXN];
                    int n = tcpRead(fd, content, MAXN);
                    if (n <= 0) {
                        break;
                    }
                    fwrite(content, sizeof(char), n, fp);
                    if (tcpWrite(fd, msgSUCCESS) <= 0) {
                        break;
                    }
                    byteRead += n;
                }
                fclose(fp);
                if (byteRead != fileSize) {
                    remove(filepath.c_str());
                }
                clientUtility.setNeedUpdateDir();
                break;
            }
            else if (command.find(msgFILEREAD) == 0u) {
                char filename[MAXN];
                unsigned long start;
                unsigned long size;
                sscanf(command.c_str() + msgFILEREAD.length(), "%*s%s%lu%lu", filename, &start, &size);
                std::string filepath = std::string("./Client/") + filename;
                struct stat fileStat;
                if (stat(filepath.c_str(), &fileStat) < 0) {
                    std::string errMsg = msgFAIL + " " + strerror(errno);
                    tcpWrite(fd, errMsg);
                    break;
                }
                if (start + size > static_cast<unsigned long>(fileStat.st_size)) {
                    std::string errMsg = msgFAIL + " Offset value error";
                    tcpWrite(fd, errMsg);
                    break;
                }
                FILE* fp = fopen(filepath.c_str(), "rb");
                if (!fp) {
                    std::string errMsg = msgFAIL + " " + strerror(errno);
                    tcpWrite(fd, errMsg);
                    break;
                }
                tcpWrite(fd, msgSUCCESS);
                fseek(fp, start, SEEK_SET);
                unsigned long byteSend = 0;
                while (byteSend < size) {
                    char content[MAXN];
                    int toRead = std::min(size - byteSend, static_cast<unsigned long>(MAXN));
                    int n = fread(content, sizeof(char), toRead, fp);
                    if (tcpWritePure(fd, content, n) <= 0) {
                        break;
                    }
                    if (tcpRead(fd, content, MAXN) <= 0) {
                        break;
                    }
                    byteSend += n;
                }
                fclose(fp);
                break;
            }
        }
    }
    close(fd);
    lb::finishThread();
}

