#ifndef NETWORK_PROGRAMMING_NPINC_HPP_
#define NETWORK_PROGRAMMING_NPINC_HPP_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <cerrno>

constexpr int MAXN = 2048;

const timeval tv10s = (timeval) {10, 0};
const timeval tv1s = (timeval) {1, 0};
const timeval tv200ms = (timeval) {0, 200000};
const timeval tv200us = (timeval) {0, 200};
const timeval tv10us = (timeval) {0, 10};

#endif // NETWORK_PROGRAMMING_NPINC_HPP_

