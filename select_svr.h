#ifndef SELECT_SVR_H
#define SELECT_SVR_H

#define _REENTRANT
#define DCE_COMPAT

#include <sys/select.h>
#include <pthread.h>

struct select_bundle
{
    int maxfd;
    int clientSize;
    int clients[FD_SETSIZE];
    fd_set set;
};

struct select_worker_arg
{
    int listenSocket;
    int bufferLength;
    struct select_bundle bundle;
};

void runSelect(int listenSocket, pthread_t *workers, const int numWorkers, const short port, const int bufferLength);
void *selectWorker(void *args);
void handleNewConnection(struct select_worker_arg *args, fd_set *set);
void handleIncomingData(struct select_worker_arg *args, fd_set *set, int num, char *buffer);

#endif // SELECT_SVR_H