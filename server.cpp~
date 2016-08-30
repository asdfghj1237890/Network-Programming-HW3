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
#include "ServerUtility.hpp"

// server data
ServerData serverData;

// server init functions
int parseArgument(int argc, const char** argv);
// server main function
void serverFunc(const int fd, ConnectInfo connectInfo);

int main(int argc, const char** argv) {
    setbuf(stdout, NULL);
    parseArgument(argc, argv);
    lb::setLogEnabled(true);
    lb::threadManageInit();
    int port = parseArgument(argc, argv);
    int listenfd = newServer(port);
    if (listenfd < 0) {
        lb::joinAll();
        return EXIT_FAILURE;
    }
    while (lb::isValid()) {
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(fileno(stdin), &fdset);
        FD_SET(listenfd, &fdset);
        timeval tv = tv200ms;
        int nready = select(std::max(fileno(stdin), listenfd) + 1, &fdset, NULL, NULL, &tv);
        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "select: %s\n", strerror(errno));
            break;
        }
        if (FD_ISSET(fileno(stdin), &fdset)) {
            char command[MAXN];
            if (fgets(command, MAXN, stdin) == NULL) {
                continue;
            }
            trimNewLine(command);
            toLowerString(command);
            if (std::string(command) == "") {
                continue;
            }
            if (std::string(command) == "q" || std::string(command) == "quit") {
                lb::setValid(false);
                break;
            }
        }
        if (FD_ISSET(listenfd, &fdset)) {
            ConnectData client = acceptConnection(listenfd);
            ConnectInfo connectInfo = getConnectInfo(client.sock);
            lb::pushThread(std::thread(serverFunc, client.fd, connectInfo));
        }
    }
    close(listenfd);
    lb::joinAll();
    return EXIT_SUCCESS;
}

int parseArgument(int argc, const char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage %s <port>\n", argv[0]);
        lb::joinAll();
        exit(EXIT_FAILURE);
    }
    int port;
    if (sscanf(argv[1], "%d", &port) != 1) {
        fprintf(stderr, "%s: not a valid port number\n", argv[1]);
        lb::joinAll();
        exit(EXIT_FAILURE);
    }
    return port;
}

void serverFunc(const int fd, ConnectInfo connectInfo) {
    ServerUtility serverUtility(fd, connectInfo);
    printLog("%sNew connection from %s port %d%s\n",
             COLOR_BRIGHT_RED, connectInfo.address.c_str(), connectInfo.port, COLOR_NORMAL);
    char buffer[MAXN];
    while (serverUtility.isValid() && lb::isValid()) {
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);
        timeval tv = tv200ms;
        int nready = select(fd + 1, &fdset, NULL, NULL, &tv);
        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            }
            printLog("select: %s\n", strerror(errno));
            break;
        }
        if (FD_ISSET(fd, &fdset)) {
            if (tcpRead(fd, buffer, MAXN) <= 0) {
                serverUtility.accountUtility(msgLOGOUT, serverData);
                break;
            }
            std::string command(buffer);
            if (command.find(msgCHECKCONNECT) == 0u) {
                tcpWrite(fd, msgCHECKCONNECT);
            }
            else if (command.find(msgREGISTER) == 0u ||
                     command.find(msgLOGIN) == 0u ||
                     command.find(msgLOGOUT) == 0u ||
                     command.find(msgDELETEACCOUNT) == 0u ||
                     command.find(msgUPDATECONNECTINFO) == 0u ||
                     command.find(msgSHOWUSER) == 0u ||
                     command.find(msgCHATREQUEST) == 0u ||
                     command.find(msgGETUSERCONN) == 0u) {
                serverUtility.accountUtility(command, serverData);
            }
            else if (command.find(msgUPDATEFILELIST) == 0u ||
                     command.find(msgSHOWFILELIST) == 0u ||
                     command.find(msgGETFILELIST) == 0u ||
                     command.find(msgFILEINFOREQUEST) == 0u) {
                serverUtility.fileUtility(command, serverData);
            }
        }
    }
    close(fd);
    lb::finishThread();
    printLog("%s%s port %d disconnected%s\n",
             COLOR_BRIGHT_RED, connectInfo.address.c_str(), connectInfo.port, COLOR_NORMAL);
}

