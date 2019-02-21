#ifndef EPOLL_SVR_H
#define EPOLL_SVR_H

void runEpoll(const int listenSocket, const short port, const int bufferLength);

void *epollWorker(void *args);

void epollSignalHandler(int sig);

#endif // EPOLL_SVR_H