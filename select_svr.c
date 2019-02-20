#include "select_svr.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "net.h"

int length;
int listenSock;

void runSelect(int listenSocket, pthread_t *workers, const int numWorkers, const short port, const int bufferLength)
{
    struct select_worker_args bundle;

    // save globals
    length = bufferLength;
    listenSock = listenSocket;

    // prepare args
    bundle.maxfd = listenSocket;
    bundle.clientSize = -1;
    FD_ZERO(&bundle.set);
    FD_SET(listenSocket, &bundle.set);
    for (int i = 0; i < FD_SETSIZE; i++)
    {
        bundle.clients[i] = -1;
    }

    for (int i = 0; i < numWorkers; i++)
    {
        pthread_create(workers + i, NULL, selectWorker, (void *)&bundle);
    }
}

void *selectWorker(void *args)
{
    int numSelected;
    char *buffer;
    fd_set readSet;

    struct select_worker_args *bundle = (struct select_worker_args *)args;

    if ((buffer = calloc(sizeof(char), length)) == NULL)
    {
        perror("Coud not allocate memory for receive buffer");
        exit(1);
    }

    while (1)
    {
        readSet = bundle->set;
        printf("[%lu] before select\n", pthread_self());
        numSelected = select(bundle->maxfd + 1, &readSet, NULL, NULL, NULL);
        printf("[%lu] after select, select returned %d\n", pthread_self(), numSelected);

        if (FD_ISSET(listenSock, &readSet))
        {
            printf("[%lu] listen socket is set\n", pthread_self());
            handleNewConnection(bundle, &readSet);

            if (--numSelected <= 0)
            {
                continue;
            }
        }

        handleIncomingData(bundle, &readSet, &numSelected, buffer);
    }

    free(buffer);

    return NULL;
}

void handleNewConnection(struct select_worker_args *bundle, fd_set *set)
{
    int i = 0;
    int newSocket = -1;
    struct sockaddr_in newClient;

    printf("[%lu] before accept\n", pthread_self());
    if (!acceptNewConnection(listenSock, &newSocket, &newClient))
    {
        perror("Coud not accept new client");
        exit(1);
    }
    printf("[%lu] after accept, new socket %d\n", pthread_self(), newSocket);

    for (i = 0; i < FD_SETSIZE; i++)
    {
        if (bundle->clients[i] < 0)
        {
            bundle->clients[i] = newSocket;
            break;
        }
    }

    if (i == FD_SETSIZE)
    {
        perror("Too many clients");
        exit(1);
    }

    FD_SET(newSocket, &bundle->set);
    if (newSocket > bundle->maxfd)
    {
        bundle->maxfd = newSocket;
    }
    if (i > bundle->clientSize)
    {
        bundle->clientSize = i;
    }
}

void handleIncomingData(struct select_worker_args *bundle, fd_set *set, int *num, char *buffer)
{
    int sock = -1;
    int dataRead = -1;

    for (int i = 0; i <= bundle->clientSize; i++)
    {
        if ((sock = bundle->clients[i]) < 0)
        {
            continue;
        }

        if (FD_ISSET(sock, set))
        {
            printf("[%lu] %d made request\n", pthread_self(), sock);
           dataRead = readAllFromSocket(sock, buffer, length);
            if (dataRead > 0)
            {
                if (sendToSocket(sock, buffer, length) == 0)
                {
                    perror("Coud not echo");
                    exit(1);
                }
                printf("[%lu] echoed to %d\n", pthread_self(), sock);
            }
            else
            {
                if (FD_ISSET(sock, &bundle->set))
                {
                    FD_CLR(sock, &bundle->set);
                    close(sock);
                    bundle->clients[i] = -1;
                    printf("[%lu] closed %d\n", pthread_self(), sock);
                }
            }
        }

        if (--(*num) <= 0)
        {
            break;
        }
    }
}