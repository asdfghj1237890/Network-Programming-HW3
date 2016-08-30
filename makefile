CC := g++

CXXFLAGS := -std=c++11 -Wall -Wextra -Os

DEFINE := -DENABLE_COLOR

.PHONY:
.PHONY: all server client clean

all: client server

client: client.cpp
	${CC} ${CXXFLAGS} ${DEFINE} $< -o $@ -pthread

server: server.cpp
	${CC} ${CXXFLAGS} ${DEFINE} $< -o $@ -pthread

clean:
	-rm -rf *.o client server

