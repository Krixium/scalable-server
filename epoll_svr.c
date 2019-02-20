#include "epoll_svr.h"

void runEpoll(int listenSocket, pthread_t *workers, const int numWorkers, const short port, const int bufferLength)
{
    // placeholder
    for (int i = 0; i < numWorkers; i++)
    {
        pthread_create(workers + i, NULL, epollWorker, NULL);
    }
}

// placeholder
void *epollWorker(void *args)
{
    return NULL;
}