#ifndef EPOLL_SVR_H
#define EPOLL_SVR_H

typedef struct
{
    int nClients;
    int epoll_fd;
    int server_fd;
    int bufLen;
    struct epoll_event* events;
} event_loop_args;

void *eventLoop(void *args);
void runEpoll(int listenSocket, const int bufferLength);
void epollSignalHandler(int sig);

#endif // EPOLL_SVR_H
