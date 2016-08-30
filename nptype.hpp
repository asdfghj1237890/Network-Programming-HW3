#ifndef NETWORK_PROGRAMMING_NPTYPE_HPP_
#define NETWORK_PROGRAMMING_NPTYPE_HPP_

#include <string>
#include <vector>
#include <map>
#include <set>

enum class NPStage {
    WELCOME, MAIN
};

struct ConnectData {
    sockaddr_in sock;
    int fd;
    ConnectData() {
        memset(&sock, 0, sizeof(sock));
        fd = -1;
    }
    ConnectData(const sockaddr_in& sock, const int fd) :
        sock(sock), fd(fd) {}
};

struct ConnectInfo {
    std::string address;
    int port;
    ConnectInfo(const std::string& address = "", const int port = -1) :
        address(address), port(port) {}
};

struct Account {
    std::string account;
    std::string password;
    ConnectInfo connectInfo;
    bool isOnline;
    Account(const std::string& account = "", const std::string& password = "", const bool isOnline = false) :
        account(account), password(password), isOnline(isOnline) {}
};

struct FileInfo {
    std::string filename;
    unsigned long size;
    std::set<std::string> owner;
    FileInfo(const std::string& filename = "", const unsigned long size = 0u) :
        filename(filename), size(size) {}
};

struct MessageBuffer {
    std::map<std::string, std::deque<std::string>> msgbuf;
};

struct ServerData {
    std::mutex dataLocker;
    std::map<std::string, Account> userData;
    std::map<std::string, FileInfo> fileData;
};

struct ClientData {
    std::mutex msgLocker;
    std::map<std::string, std::deque<std::string>> msg;
};

#endif // NETWORK_PROGRAMMING_NPTYPE_HPP_

