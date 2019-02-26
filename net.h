#ifndef NET_H
#define NET_H

#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>

bool setSocketToReuse(int sock);
bool setSocketToNonBlocking(int sock);
bool setSocketTimeout(const size_t sec, const size_t usec, const int sock);
bool createTCPSocket(int *sock);
bool createBoundSocket(int *sock, const short port);
bool acceptNewConnection(const int listenSocket, int *newSocket, struct sockaddr_in *client);
int readAllFromSocket(const int sock, char *buffer, const int size);
int sendToSocket(const int sock, char *buffer, const int size);
int clearSocket(int socket, char* buf, const int len);

#endif // NET_H