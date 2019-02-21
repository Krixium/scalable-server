#ifndef SELECT_SVR_H
#define SELECT_SVR_H

#define _REENTRANT
#define DCE_COMPAT

#include <sys/select.h>
#include <pthread.h>

struct select_worker_args
{
    int listenSocket;
    int maxfd;
    int clientSize;
    int clients[FD_SETSIZE];
    fd_set set;
};

void runSelect(int listenSocket, pthread_t *workers, const int numWorkers, const short port, const int bufferLength);

void *selectWorker(void *args);
void handleNewConnection(struct select_worker_args *bundle, fd_set *set);
void handleIncomingData(struct select_worker_args *bundle, fd_set *set, int *num, char *buffer);

#endif // SELECT_SVR_H
