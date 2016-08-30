#ifndef NETWORK_PROGRAMMING_THREADUTIL_HPP_
#define NETWORK_PROGRAMMING_THREADUTIL_HPP_

#include <cstdio>
#include <sstream>
#include <chrono>
#include <list>
#include <utility>
#include <thread>
#include <mutex>
#include "nputility.hpp"

namespace lb {

// thread related variables
bool valid;
bool logEnabled;
std::mutex threadLocker;
// vector of threads
std::list<std::pair<std::thread, bool>> threads;
// getter, setter
void setLogEnabled(const bool val);
void setValid(const bool val);
bool isValid();
// thread related functions
void threadManageInit();
void threadMaintain();
void pushThread(std::thread&& item);
void finishThread();
void joinAll();
std::string getThreadIdStr(const std::thread::id id);

void setLogEnabled(const bool val) {
    logEnabled = val;
}

void setValid(const bool val) {
    valid = val;
}

bool isValid() {
    return valid;
}

void threadManageInit() {
    valid = true;
    pushThread(std::thread(threadMaintain));
}

void threadMaintain() {
    while (valid) {
        threadLocker.lock();
        bool flag = true;
        while (flag) {
            flag = false;
            for (auto it = threads.begin(); it != threads.end(); ++it) {
                if (!it->second) {
                    flag = true;
                    std::string threadIdStr = getThreadIdStr(it->first.get_id());
                    it->first.join();
                    threads.erase(it);
                    if (logEnabled) {
                        printLog("Thread id %s finished\n", threadIdStr.c_str());
                    }
                    break;
                }
            }
        }
        threadLocker.unlock();
        if (valid) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void pushThread(std::thread&& item) {
    threadLocker.lock();
    if (logEnabled) {
        printLog("Thread id %s started\n", getThreadIdStr(item.get_id()).c_str());
    }
    threads.push_back(std::make_pair(std::move(item), true));
    threadLocker.unlock();
}

void finishThread() {
    std::lock_guard<std::mutex> lock(threadLocker);
    for (auto& item : threads) {
        if (std::this_thread::get_id() == item.first.get_id()) {
            item.second = false;
            break;
        }
    }
}

void joinAll() {
    valid = false;
    for (auto& item : threads) {
        if (item.first.joinable()) {
            if (logEnabled) {
                printLog("Thread id %s finished\n",getThreadIdStr(item.first.get_id()).c_str());
            }
            item.first.join();
        }
    }
}

std::string getThreadIdStr(const std::thread::id id) {
    std::ostringstream oss;
    oss << std::hex << id;
    return oss.str();
}

} // namespace lb

#endif // NETWORK_PROGRAMMING_THREADUTIL_HPP_

