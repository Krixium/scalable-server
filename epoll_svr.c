#include "epoll_svr.h"

#define _REENTRANT
#define DCE_COMPAT

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <unistd.h>

pthread_t *workers;

void runEpoll(const int listenSocket, const short port, const int bufferLength)
{
    signal(SIGINT, epollSignalHandler);

    if ((workers = calloc(get_nprocs(), sizeof(pthread_t))) == NULL)
    {
        perror("Coudl not allocate workers");
        exit(1);
    }

    // placeholder
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_create(workers + i, NULL, epollWorker, NULL);
    }
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_join(workers[i], NULL);
    }

}

// placeholder
void *epollWorker(void *args)
{
    return NULL;
}

void epollSignalHandler(int sig)
{
    fprintf(stdout, "Stopping server\n");
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_cancel(workers[i]);
    }
}