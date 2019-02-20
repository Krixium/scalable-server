#include "select_svr.h"

void runSelect(int listenSocket, pthread_t *workers, const int numWorkers, const short port, const int bufferLength)
{
    // placeholder
    for (int i = 0; i < numWorkers; i++)
    {
        pthread_create(workers + i, NULL, selectWorker, NULL);
    }
}

// placeholder
void *selectWorker(void *args)
{
    return NULL;
}