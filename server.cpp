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

#ifndef COLORCODES_H_
#define COLORCODES_H_

#ifdef ENABLE_COLOR

#define COLOR_NORMAL          "\x1B[0m"
#define COLOR_RED             "\x1B[31m"
#define COLOR_GREEN           "\x1B[32m"
#define COLOR_YELLOW          "\x1B[33m"
#define COLOR_BLUE            "\x1B[34m"
#define COLOR_MAGENTA         "\x1B[35m"
#define COLOR_CYAN            "\x1B[36m"
#define COLOR_WHITE           "\x1B[37m"
#define COLOR_BRIGHT_RED      "\x1B[1;31m"
#define COLOR_BRIGHT_GREEN    "\x1B[1;32m"
#define COLOR_BRIGHT_YELLOW   "\x1B[1;33m"
#define COLOR_BRIGHT_BLUE     "\x1B[1;34m"
#define COLOR_BRIGHT_MAGENTA  "\x1B[1;35m"
#define COLOR_BRIGHT_CYAN     "\x1B[1;36m"
#define COLOR_BRIGHT_WHITE    "\x1B[1;37m"

#else

#define COLOR_NORMAL          ""
#define COLOR_RED             ""
#define COLOR_GREEN           ""
#define COLOR_YELLOW          ""
#define COLOR_BLUE            ""
#define COLOR_MAGENTA         ""
#define COLOR_CYAN            ""
#define COLOR_WHITE           ""
#define COLOR_BRIGHT_RED      ""
#define COLOR_BRIGHT_GREEN    ""
#define COLOR_BRIGHT_YELLOW   ""
#define COLOR_BRIGHT_BLUE     ""
#define COLOR_BRIGHT_MAGENTA  ""
#define COLOR_BRIGHT_CYAN     ""
#define COLOR_BRIGHT_WHITE    ""

#endif // ENABLE_COLOR

#endif // COLORCODES_H_


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

