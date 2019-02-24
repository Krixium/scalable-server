#include "epoll_svr.h"

#define _REENTRANT
#define DCE_COMPAT

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
static const int EPOLL_FLAGS = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;

pthread_t *workers;

void *eventLoop(void *args)
{
    struct epoll_event current_event, event;
    int n_ready;
    int status;
    struct sockaddr_in remote_addr;
    char *local_buffer;
    int epoll_fd;
    struct epoll_event events[MAX_EVENTS];

    event_loop_args *ev_args = (event_loop_args *)args;

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        systemFatal("epoll_create1");
    }

    // Register the server socket for epoll events
    event.data.fd = ev_args->server_fd;
    event.events = EPOLLIN | EPOLLET;
    status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev_args->server_fd, &event);
    if (status == -1)
    {
        systemFatal("epoll_ctl");
    }

    if ((local_buffer = calloc(ev_args->bufLen, sizeof(char))) == NULL)
    {
        systemFatal("calloc");
    }

    while (true)
    {
        n_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_ready == -1)
        {
            systemFatal("epoll_wait");
        }

        // A file descriptor is ready
        for (int i = 0; i < n_ready; ++i)
        {
            int client_fd;
            socklen_t remote_len;
            current_event = events[i];
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
                client_fd = accept(ev_args->server_fd, (struct sockaddr*) &remote_addr, &remote_len);
                if (client_fd == -1) {
                    if (errno != EAGAIN)
                    {
                        perror("accept");
                    }
                    break;
                }
                logAcc(client_fd);

                status = fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);
                if (status == -1)
                {
                    perror("fcntl");
                    close(client_fd);
                    continue;
                }

                // Add the client socket to the epoll instance
                event.data.fd = client_fd;
                event.events = EPOLL_FLAGS;
                status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                if (status == -1)
                {
                    close(client_fd);
                    if (errno == EBADF) // this is a hack
                    {
                        continue;
                    }
                    systemFatal("epoll_ctl");
                }
            }
            else
            {
                if (!clearSocket(current_event.data.fd, local_buffer, ev_args->bufLen))
                {
                    close(current_event.data.fd);
                }
            }
        }
    }

    free(local_buffer);
    close(epoll_fd);
}

void runEpoll(int listenSocket, const short port, const int bufferLength)
{
    event_loop_args *args = calloc(1, sizeof(event_loop_args));

    signal(SIGINT, epollSignalHandler);

    args->server_fd = listenSocket;

    setSocketToNonBlocking(args->server_fd);

    args->bufLen = (size_t)bufferLength;

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

    //eventLoop((void*) args);

    free(args);
}

void epollSignalHandler(int sig)
{
    fprintf(stdout, "Stopping server\n");
    for (int i = 0; i < get_nprocs(); i++)
    {
       pthread_cancel(workers[i]);
    }
}
