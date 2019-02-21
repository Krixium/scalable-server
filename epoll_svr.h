#ifndef EPOLL_SVR_H
#define EPOLL_SVR_H

#define _REENTRANT
#define DCE_COMPAT

#include <pthread.h>

void runEpoll(int listenSocket, pthread_t *workers, const int numWorkers, const short port, const int bufferLength);

void *epollWorker(void *args);

#endif // EPOLL_SVR_H
