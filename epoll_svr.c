/*---------------------------------------------------------------------------------------
-- SOURCE FILE:            epoll_svr.c
--
-- PROGRAM:                server.out
--
-- FUNCTIONS:
--                         void *eventLoop(void *args)
--                         void runEpoll(int listenSocket, const int bufferLength)
--                         void epollSignalHandler(int sig)
--
-- DATE:                   Feb 19, 2019
--
-- REVISIONS:              N/A
--
-- DESIGNERS:              Benny Wang, William Murpy
--
-- PROGRAMMERS:            William Murphy
--
-- NOTES:
-- Contains all functions for running the server in epoll mode.
---------------------------------------------------------------------------------------*/
#define _REENTRANT
#define DCE_COMPAT

#include "epoll_svr.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#include "tools.h"
#include "net.h"

// Globals
static const int MAX_EVENTS = 256;
pthread_t *workers;

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                eventLoop
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                William Murphy
--
-- PROGRAMMER:              William Murphy
--
-- INTERFACE:               void *eventLoop(void *args)
--
-- RETURNS:                 NULL - unused.
--
-- NOTES:
-- The main function of each worker thread for epoll. Allocates a buffer for storing data and then
-- goes into a forever loop that blocks on epoll_wait and then handles requests accordingly.
--------------------------------------------------------------------------------------------------*/
void *eventLoop(void *args)
{
    struct epoll_event current_event, event;
    int n_ready;
    int status;
    struct sockaddr_in remote_addr;
    char *local_buffer;

    event_loop_args *ev_args = (event_loop_args *)args;

    if ((local_buffer = calloc(ev_args->bufLen, sizeof(char))) == NULL)
    {
        systemFatal("calloc");
    }

    while (true)
    {
        n_ready = epoll_wait(ev_args->epoll_fd, ev_args->events, MAX_EVENTS, -1);
        if (n_ready == -1)
        {
            systemFatal("epoll_wait");
        }

        // A file descriptor is ready
        for (int i = 0; i < n_ready; ++i)
        {
            int client_fd;
            current_event = ev_args->events[i];
            // An error or hangup occurred
            // Client might have closed their side of the connection
            if (current_event.events & (EPOLLHUP | EPOLLERR))
            {
                fprintf(stderr, "epoll: EPOLLERR or EPOLLHUP\n");
                close(current_event.data.fd);
                continue;
            }

            // The server is receiving a connection request
            if (current_event.data.fd == ev_args->server_fd)
            {
                if (!acceptNewConnection(ev_args->server_fd, &client_fd, &remote_addr))
                {
                    continue;
                }

                if (!setSocketToNonBlocking(client_fd))
                {
                    systemFatal("setSocketToNonBlocking");
                }

                // Add the client socket to the epoll instance
                event.data.fd = client_fd;
                event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
                status = epoll_ctl(ev_args->epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                if (status == -1)
                {
                    systemFatal("epoll_ctl");
                }

                continue;
            }

            if (current_event.events & EPOLLIN)
            {
                if (!clearSocket(current_event.data.fd, local_buffer, ev_args->bufLen))
                {
                    close(current_event.data.fd);
                }
            }
        }
    }

    free(local_buffer);
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                runEpoll
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                William Murphy
--
-- PROGRAMMER:              William Murphy
--
-- INTERFACE:               void runEpoll(int listenSocket, const int bufferLength)
--
-- NOTES:
-- The main entry point for epoll mode. Prepares the arguments for epoll and then spawns one worker
-- thread per cpu and waits for all workers to exit.
--------------------------------------------------------------------------------------------------*/
void runEpoll(int listenSocket, const int bufferLength)
{
    int status;
    struct epoll_event event;
    event_loop_args *args = calloc(1, sizeof(event_loop_args));

    signal(SIGINT, epollSignalHandler);

    args->server_fd = listenSocket;

    setSocketToNonBlocking(args->server_fd);

    args->epoll_fd = epoll_create1(0); // might need flags
    if (args->epoll_fd == -1)
    {
        systemFatal("epoll_create1");
    }

    // Register the server socket for epoll events
    event.data.fd = args->server_fd;
    event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    status = epoll_ctl(args->epoll_fd, EPOLL_CTL_ADD, args->server_fd, &event);
    if (status == -1)
    {
        systemFatal("epoll_ctl");
    }

    args->bufLen = (size_t)bufferLength;
    args->events = calloc(MAX_EVENTS, sizeof(struct epoll_event));

    if ((workers = calloc(get_nprocs(), sizeof(pthread_t))) == NULL)
    {
        systemFatal("calloc");
    }

    // Start the event loop
    for (int i = 0; i < get_nprocs(); i++)
    {
        if (pthread_create(workers + i, NULL, eventLoop, (void *)args))
        {
            systemFatal("pthread_create");
        }
    }
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_join(workers[i], NULL);
    }

    free(args->events);
    free(args);
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                epollSignalHandler
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                William Murphy
--
-- PROGRAMMER:              William Murphy
--
-- INTERFACE:               void epollSignalHandler(int sig)
--                              int sig: The signal that was caught.
--
-- NOTES:
-- Callback function to catch SIGINT. Kills all the worker threads.
--------------------------------------------------------------------------------------------------*/
void epollSignalHandler(int sig)
{
    fprintf(stdout, "Stopping server\n");
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_cancel(workers[i]);
    }
}
