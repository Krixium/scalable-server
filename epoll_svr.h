#ifndef EPOLL_SVR_H
#define EPOLL_SVR_H

typedef struct
{
    int nClients;
    int server_fd;
    int bufLen;
} event_loop_args;

void *eventLoop(void *args);
void runEpoll(int listenSocket, const int bufferLength);
void epollSignalHandler(int sig);

#endif // EPOLL_SVR_H
