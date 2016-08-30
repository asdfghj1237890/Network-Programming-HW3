#ifndef NETWORK_PROGRAMMING_SERVERUTILITY_HPP_
#define NETWORK_PROGRAMMING_SERVERUTILITY_HPP_

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

class ServerUtility {
public:
    ServerUtility(int fd, ConnectInfo connectInfo) {
        this->fd = fd;
        this->connectInfo = connectInfo;
        this->valid = true;
    }

    ~ServerUtility() {

    }

    bool isValid() const {
        return valid;
    }

    void accountUtility(const std::string& msg, ServerData& data) {
        std::lock_guard<std::mutex> lock(data.dataLocker);
        if (msg.find(msgREGISTER) == 0u) {
            accountRegister(msg, data);
        }
        else if (msg.find(msgLOGIN) == 0u) {
            accountLogin(msg, data);
        }
        else if (msg.find(msgLOGOUT) == 0u) {
            accountLogout(msg, data);
        }
        else if (msg.find(msgDELETEACCOUNT) == 0u) {
            accountDelete(msg, data);
        }
        else if (msg.find(msgUPDATECONNECTINFO) == 0u) {
            accountUpdateConnectInfo(msg, data);
        }
        else if (msg.find(msgSHOWUSER) == 0u) {
            accountShowInfo(msg, data);
        }
        else if (msg.find(msgCHATREQUEST) == 0u) {
            accountSendConnectInfo(msg, data);
        }
        else if (msg.find(msgGETUSERCONN) == 0u) {
            accountGetConnectInfo(msg, data);
        }
    }

    void fileUtility(const std::string& msg, ServerData& data) {
        std::lock_guard<std::mutex> lock(data.dataLocker);
        if (msg.find(msgUPDATEFILELIST) == 0u) {
            fileListUpdate(msg, data);
        }
        else if (msg.find(msgSHOWFILELIST) == 0u) {
            fileListShow(msg, data);
        }
        else if (msg.find(msgGETFILELIST) == 0u) {
            fileListGet(msg, data);
        }
        else if (msg.find(msgFILEINFOREQUEST) == 0u) {
            fileInfoRequest(msg, data);
        }
    }

private:
    // REGISTER account password
    void accountRegister(const std::string& msg, ServerData& data) {
        char account[MAXN];
        char password[MAXN];
        sscanf(msg.c_str() + msgREGISTER.length(), "%s%s", account, password);
        if (data.userData.count(account)) {
            std::string reply = msgFAIL + " Account already exists";
            tcpWrite(fd, reply);
        }
        else {
            printLog("New account %s created\n", account);
            data.userData.insert(std::make_pair(account, Account(account, password)));
            std::string reply = msgSUCCESS;
            tcpWrite(fd, reply);
        }
    }

    // LOGIN account password
    void accountLogin(const std::string& msg, ServerData& data) {
        char account[MAXN];
        char password[MAXN];
        sscanf(msg.c_str() + msgLOGIN.length(), "%s%s", account, password);
        if (!data.userData.count(account) || data.userData.at(account).password != std::string(password)) {
            std::string reply = msgFAIL + " Invalid account or password";
            tcpWrite(fd, reply);
        }
        else if (data.userData.at(account).isOnline) {
            std::string reply = msgFAIL + " Already online, please log out first";
            tcpWrite(fd, reply);
        }
        else {
            nowAccount = account;
            data.userData[account].isOnline = true;
            std::string reply = msgSUCCESS;
            tcpWrite(fd, reply);
        }
    }

    // LOGOUT
    void accountLogout(const std::string& msg, ServerData& data) {
        if (msg.find(msgLOGOUT) != 0u || nowAccount == "") {
            return;
        }
        printLog("Account %s logout\n", nowAccount.c_str());
        data.userData[nowAccount].isOnline = false;
        nowAccount = "";
    }

    // DELETEACCOUNT
    void accountDelete(const std::string& msg, ServerData& data) {
        if (msg.find(msgDELETEACCOUNT) != 0u) {
            return;
        }
        data.userData.erase(nowAccount);
        cleanAccountFileList(nowAccount, data);
        printLog("Account %s was deleted\n", nowAccount.c_str());
        nowAccount = "";
    }

    // UPDATECONNECTIONINFO port
    void accountUpdateConnectInfo(const std::string& msg, ServerData& data) {
        int port;
        sscanf(msg.c_str() + msgUPDATECONNECTINFO.length(), "%d", &port);
        data.userData[nowAccount].connectInfo = ConnectInfo(connectInfo.address, port);
        printLog("Account %s info updated IP %s port %d\n", nowAccount.c_str(), connectInfo.address.c_str(), port);
    }

    // SHOWUSER
    void accountShowInfo(const std::string& msg, ServerData& data) {
        if (msg.find(msgSHOWUSER) != 0u) {
            return;
        }
        std::string reply = "Online Users:\n";
        //reply += COLOR_BRIGHT_GREEN;
        reply += "    Account                     IP                Port";
        //reply += COLOR_NORMAL;
        reply += "\n";
        for (auto& item : data.userData) {
            if (item.second.isOnline) {
                char formatBuffer[MAXN];
                snprintf(formatBuffer, MAXN, "    %-25s   %-15s   %d\n",
                         item.first.c_str(),
                         item.second.connectInfo.address.c_str(),
                         item.second.connectInfo.port);
                reply += formatBuffer;
            }
        }
        tcpWrite(fd, reply);
    }

    // CHATREQUEST account
    void accountSendConnectInfo(const std::string& msg, ServerData& data) {
        char account[MAXN];
        sscanf(msg.c_str() + msgCHATREQUEST.length(), "%s", account);
        printLog("%s requested connection info of account %s\n", nowAccount.c_str(), account);
        if (!data.userData.count(account)) {
            std::string reply = msgFAIL + " User not found";
            tcpWrite(fd, reply);
        }
        else if (!data.userData[account].isOnline) {
            std::string reply = msgFAIL + " User is not online";
            tcpWrite(fd, reply);
        }
        else {
            std::string reply = msgSUCCESS;
            reply += " " + data.userData[account].connectInfo.address;
            reply += " " + std::to_string(data.userData[account].connectInfo.port);
            tcpWrite(fd, reply);
        }
    }

    // GETUSERCONN account
    void accountGetConnectInfo(const std::string& msg, ServerData& data) {
        char account[MAXN];
        sscanf(msg.c_str() + msgGETUSERCONN.length(), "%s", account);
        printLog("%s requested connection info of account %s\n", nowAccount.c_str(), account);
        if (!data.userData.count(account)) {
            std::string reply = msgFAIL + " User not found";
            tcpWrite(fd, reply);
        }
        else if (!data.userData[account].isOnline) {
            std::string reply = msgFAIL + " User is not online";
            tcpWrite(fd, reply);
        }
        else {
            std::string reply = msgSUCCESS + " " +
                                data.userData[account].connectInfo.address + " " +
                                std::to_string(data.userData[account].connectInfo.port);
            tcpWrite(fd, reply);
        }
    }

    // UPDATEFILELIST [files ...]
    void fileListUpdate(const std::string& msg, ServerData& data) {
        cleanAccountFileList(nowAccount, data);
        std::istringstream iss(msg.c_str() + msgUPDATEFILELIST.length());
        std::string filename;
        unsigned long filesize;
        printLog("Account %s files:\n", nowAccount.c_str());
        while (iss >> filename >> filesize) {
            data.fileData[filename].filename = filename;
            data.fileData[filename].size = filesize;
            data.fileData[filename].owner.insert(nowAccount);
            printf("          %s (%lu bytes)\n", filename.c_str(), filesize);
        }
    }

    // SHOWFILELIST
    void fileListShow(const std::string& msg, ServerData& data) {
        if (msg.find(msgSHOWFILELIST) != 0u) {
            return;
        }
        std::string reply = "Files:\n";
        for (auto& item : data.fileData) {
            //reply += COLOR_BRIGHT_GREEN;
            reply += "    " + item.first + " (" + std::to_string(item.second.size) + " bytes)";
            //reply += COLOR_NORMAL;
            reply += "\n";
            for (auto& who : item.second.owner) {
                if (!data.userData[who].isOnline) {
                    continue;
                }
                reply += "        " + who + ((data.userData[who].isOnline) ? " [Online]" : " [Offline]") + "\n";
            }
            for (auto& who : item.second.owner) {
                if (data.userData[who].isOnline) {
                    continue;
                }
                reply += "        " + who + ((data.userData[who].isOnline) ? " [Online]" : " [Offline]") + "\n";
            }
        }
        tcpWrite(fd, reply);
    }

    // GETFILELIST account
    void fileListGet(const std::string& msg, ServerData& data) {
        char account[MAXN];
        sscanf(msg.c_str() + msgGETFILELIST.length(), "%s", account);
        if (!data.userData.count(account)) {
            std::string reply = msgFAIL + " User not found";
            tcpWrite(fd, reply);
        }
        else {
            std::string reply = std::string("Account: ") + account;
            reply += std::string(" ") + (data.userData[account].isOnline ? "[Online]" : "[Offline]") + "\n";
            for (auto& item : data.fileData) {
                if (item.second.owner.count(account)) {
                    //reply += COLOR_BRIGHT_GREEN;
                    reply += "    " + item.first;
                    //reply += COLOR_NORMAL;
                    reply += " (" + std::to_string(item.second.size) + " bytes)\n";
                }
            }
            tcpWrite(fd, reply);
        }
    }

    // FILEINFOREQUEST DIRECT|P2P account filename
    void fileInfoRequest(const std::string& msg, ServerData& data) {
        char option[MAXN];
        char account[MAXN];
        char filename[MAXN];
        sscanf(msg.c_str() + msgFILEINFOREQUEST.length(), "%s", option);
        if (std::string(option) == "DIRECT") {
            sscanf(msg.c_str() + msgFILEINFOREQUEST.length(), "%*s%s%s", account, filename);
            printLog("Account %s requested connection info of account %s\n", nowAccount.c_str(), account);
            printf("          Direct Download File %s\n", filename);
            if (!data.userData.count(account)) {
                std::string reply = msgFAIL + " User not found";
                tcpWrite(fd, reply);
                return;
            }
            else if (!data.userData[account].isOnline) {
                std::string reply = msgFAIL + " User is not online";
                tcpWrite(fd, reply);
                return;
            }
            else if (!data.fileData.count(filename)) {
                std::string reply = msgFAIL + " File not found in server database";
                tcpWrite(fd, reply);
                return;
            }
            else if (!data.fileData[filename].owner.count(account)) {
                std::string reply = msgFAIL + " User doesn\'t has the file";
                tcpWrite(fd, reply);
                return;
            }
            std::string reply = msgSUCCESS + " " +
                                data.userData[account].connectInfo.address + " " +
                                std::to_string(data.userData[account].connectInfo.port) + " " +
                                std::to_string(data.fileData[filename].size);
            tcpWrite(fd, reply);
        }
        else {
            char filename[MAXN];
            sscanf(msg.c_str() + msgFILEINFOREQUEST.length(), "%*s%s", filename);
            printLog("Account %s requested connection info\n", nowAccount.c_str());
            printf("          P2P Download File %s\n", filename);
            if (!data.fileData.count(filename)) {
                std::string reply = msgFAIL + " File not found in server database";
                tcpWrite(fd, reply);
                return;
            }
            std::vector<std::string> targetUsers;
            for (const auto& who : data.fileData[filename].owner) {
                if (data.userData[who].isOnline && who != nowAccount) {
                    targetUsers.push_back(who);
                }
            }
            if (targetUsers.empty()) {
                std::string reply = msgFAIL + " No avaliable users to download from(all offline)";
                tcpWrite(fd, reply);
                return;
            }
            std::string reply = msgSUCCESS + " " +
                                std::to_string(data.fileData[filename].size);
            unsigned long fileSize = data.fileData[filename].size;
            unsigned long offset = 0;
            unsigned long div = data.fileData[filename].size / targetUsers.size();
            for (auto i = 0lu; i < targetUsers.size(); ++i) {
                const std::string& who = targetUsers[i];
                reply += " " + data.userData[who].connectInfo.address;
                reply += " " + std::to_string(data.userData[who].connectInfo.port);
                if (i == targetUsers.size() - 1 || div == 0) {
                    printf("          Account %s offset %lu size %lu\n", who.c_str(), offset, fileSize - offset);
                    reply += " " + std::to_string(offset) + " " + std::to_string(fileSize);
                    break;
                }
                printf("          Account %s offset %lu size %lu\n", who.c_str(), offset, div);
                reply += " " + std::to_string(offset) + " " + std::to_string(offset + div);
                offset += div;
            }
            tcpWrite(fd, reply);
        }
    }

private:
    void cleanAccountFileList(const std::string account, ServerData& data) {
        for (auto& item : data.fileData) {
            item.second.owner.erase(account);
        }
        while (true) {
            bool flag = false;
            std::string filename;
            for (auto& item : data.fileData) {
                if (item.second.owner.empty()) {
                    filename = item.first;
                    flag = true;
                    break;
                }
            }
            if (flag) {
                data.fileData.erase(filename);
            }
            else {
                break;
            }
        }
    }

private:
    std::string nowAccount;
    ConnectInfo connectInfo;
    int fd;
    bool valid;
};

#endif // NETWORK_PROGRAMMING_SERVERUTILITY_HPP_

