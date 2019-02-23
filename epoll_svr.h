#ifndef EPOLL_SVR_H
#define EPOLL_SVR_H

#include <stdbool.h>

typedef struct
{
    int nClients;
    int epoll_fd;
    int server_fd;
    int bufLen;
    struct epoll_event* events;
} event_loop_args;

void *eventLoop(void *args);
bool clearSocket(int socket, char *buf, const int len);
void runEpoll(int listenSocket, const short port, const int bufferLength);
void epollSignalHandler(int sig);

#endif // EPOLL_SVR_H
