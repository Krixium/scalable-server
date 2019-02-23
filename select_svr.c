#include "select_svr.h"

#define _REENTRANT
#define DCE_COMPAT

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include "tools.h"
#include "net.h"

pthread_t *workers;

void runSelect(const int listenSocket, const short port, const int bufferLength)
{
    struct select_worker_arg arg;
    arg.listenSocket = listenSocket;

    if (!setSocketToNonBlocking(arg.listenSocket))
    {
        systemFatal("setSocketToNonBlocking");
    }

    signal(SIGINT, selectSignalHandler);

    if ((workers = calloc(get_nprocs(), sizeof(pthread_t))) == NULL)
    {
        systemFatal("calloc");
    }

    // prepare arg
    arg.bufferLength = bufferLength;
    arg.bundle.maxfd = listenSocket;
    arg.bundle.clientSize = -1;
    for (int i = 0; i < FD_SETSIZE; i++)
    {
        arg.bundle.clients[i] = -1;
    }
    FD_ZERO(&arg.bundle.set);
    FD_SET(listenSocket, &arg.bundle.set);

    for (int i = 0; i < get_nprocs(); i++)
    {
        if (pthread_create(workers + i, NULL, selectWorker, (void *)&arg))
        {
            systemFatal("pthread_create");
        }
    }

    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_join(workers[i], NULL);
    }
}

void *selectWorker(void *args)
{
    int numSelected;
    char *buffer;
    fd_set readSet;

    struct select_worker_arg *argPtr = (struct select_worker_arg *)args;

    if ((buffer = calloc(sizeof(char), argPtr->bufferLength)) == NULL)
    {
        systemFatal("calloc");
    }

    while (true)
    {
        readSet = argPtr->bundle.set;
        numSelected = select(argPtr->bundle.maxfd + 1, &readSet, NULL, NULL, NULL);

        if (FD_ISSET(argPtr->listenSocket, &readSet))
        {
            handleNewConnection(argPtr, &readSet);

            if (--numSelected <= 0)
            {
                continue;
            }
        }

        handleIncomingData(argPtr, &readSet, numSelected, buffer);
    }

    free(buffer);

    return NULL;
}

void handleNewConnection(struct select_worker_arg *args, fd_set *set)
{
    int i = 0;
    int newSocket = -1;
    struct sockaddr_in newClient;

    struct select_worker_arg *argPtr = (struct select_worker_arg *)args;

    if (!acceptNewConnection(argPtr->listenSocket, &newSocket, &newClient))
    {
        return;
    }

    for (i = 0; i < FD_SETSIZE; i++)
    {
        if (argPtr->bundle.clients[i] < 0)
        {
            argPtr->bundle.clients[i] = newSocket;
            break;
        }
    }

    if (i == FD_SETSIZE)
    {
        systemFatal("Too many clients");
    }

    FD_SET(newSocket, &argPtr->bundle.set);
    if (newSocket > argPtr->bundle.maxfd)
    {
        argPtr->bundle.maxfd = newSocket;
    }
    if (i > argPtr->bundle.clientSize)
    {
        argPtr->bundle.clientSize = i;
    }
}

void handleIncomingData(struct select_worker_arg *args, fd_set *set, int num, char *buffer)
{
    int sock = -1;

    struct select_worker_arg *argPtr = (struct select_worker_arg *)args;

    for (int i = 0; i <= argPtr->bundle.clientSize; i++)
    {
        if ((sock = argPtr->bundle.clients[i]) < 0)
        {
            continue;
        }

        if (FD_ISSET(sock, set))
        {
            if (!clearSocket(sock, buffer, argPtr->bufferLength))
            {
                FD_CLR(sock, &argPtr->bundle.set);
                argPtr->bundle.clients[i] = -1;
            }
        }

        if (--num <= 0)
        {
            break;
        }
    }
}

void selectSignalHandler(int sig)
{
    fprintf(stdout, "Stopping server\n");
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_cancel(workers[i]);
    }
}
