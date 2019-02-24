/*---------------------------------------------------------------------------------------
-- SOURCE FILE:            select_svr.c
--
-- PROGRAM:                server.out
--
-- FUNCTIONS:
--                         void runSelect(const int listenSocket, const int bufferLength)
--                         void *selectWorker(void *args)
--                         void handleNewConnection(struct select_worker_arg *args)
--                         void handleIncomingData(struct select_worker_arg *args, fd_set *set, int num, char *buffer)
--                         void selectSignalHandler(int sig)
--
-- DATE:                   Feb 19, 2019
--
-- REVISIONS:              N/A
--
-- DESIGNERS:              Benny Wang, William Murpy
--
-- PROGRAMMERS:            Benny Wang
--
-- NOTES:
-- Contains all functions used for running the server in select mode.
---------------------------------------------------------------------------------------*/
#define _REENTRANT
#define DCE_COMPAT

#include "select_svr.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include "tools.h"
#include "net.h"

pthread_t *workers;

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                runSelect
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void runSelect(const int listenSocket, const int bufferLength)
--                              const int listenSocket: The listening socket.
--                              const int bufferLength: The size of the buffer.
--
-- NOTES:
-- The main entry point for select mode. Prepares the arguments for select and then spawns one worker
-- thread per cpu and waits for all workers to exit.
--------------------------------------------------------------------------------------------------*/
void runSelect(const int listenSocket, const int bufferLength)
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

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                selectWorker
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void *selectWorker(void *args)
--                              void *args: A select_workera_arg struct.
--
-- RETURNS:                 NULL - unused.
--
-- NOTES:
-- The main function of each worker thread for select. Allocates a buffer for storing data and then
-- goes into a forever loop that blocks on select and then handles requests accordingly.
--------------------------------------------------------------------------------------------------*/
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
            handleNewConnection(argPtr);

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

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                handleNewConnection
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void handleNewConnection(struct select_worker_arg *args)
--                              struct select_worker_arg *args: The worker arguments.
--
-- NOTES:
-- Accepts a new connection and set the associated select parameters.
--------------------------------------------------------------------------------------------------*/
void handleNewConnection(struct select_worker_arg *args)
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

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                handleIncomingData
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void handleIncomingData(struct select_worker_arg *args, fd_set *set, int num, char *buffer)
--                              struct select_worker_arg *args: The worker arguments.
--                              fd_set *set: The current set.
--                              int num: The number of set bits in set.
--                              char *buffer: The buffer to store the data.
--
-- NOTES:
-- Handles all the data sockets that were flagged by the select call.
--------------------------------------------------------------------------------------------------*/
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

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                selectSignalHandler
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void selectSignalHandler(int sig)
--                             int sig: The signal that was caught.
--
-- NOTES:
-- Callback function to catch SIGINT. Kills all the worker threads.
--------------------------------------------------------------------------------------------------*/
void selectSignalHandler(int sig)
{
    fprintf(stdout, "Stopping server\n");
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_cancel(workers[i]);
    }
}
