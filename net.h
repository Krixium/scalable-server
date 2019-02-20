#ifndef NET_H
#define NET_H

#include <arpa/inet.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>

int setSocketToReuse(int *sock);
int createTCPSocket(int *sock);
int createBoundSocket(int *sock, const short port);
int acceptNewConnection(const int listenSocket, int *newSocket, struct sockaddr_in *client);
int readAllFromSocket(const int sock, char *buffer, const int size);
int sendToSocket(const int sock, char *buffer, const int size);

#endif // NET_H