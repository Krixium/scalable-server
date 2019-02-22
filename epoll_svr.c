#include "epoll_svr.h"

#define _REENTRANT
#define DCE_COMPAT

#include <pthread.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include "net.h"

pthread_t *workers;

void *eventLoop(void *args)
{
    const int MAX_EVENTS = 256;

    struct epoll_event current_event, event;
    int n_ready;
    int status;
    struct sockaddr_storage remote_addr;
    socklen_t addr_len;

    event_loop_args *ev_args = (event_loop_args *)args;

    int server_fd = ev_args->server_fd;
    int epoll_fd = ev_args->epoll_fd;
    const int bufLen = ev_args->bufLen;

    char *buffer = ev_args->buffer;
    struct epoll_event *events = calloc(MAX_EVENTS, sizeof(struct epoll_event));

    while (true)
    {
        n_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_ready == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        // A file descriptor is ready
        for (int i = 0; i < n_ready; ++i)
        {
            int client_fd;
            current_event = events[i];
            // An error or hangup occurred
            // Client might have closed their side of the connection
            if (current_event.events & (EPOLLHUP | EPOLLERR))
            {
                fprintf(stderr, "epoll: EPOLLERR or EPOLLHUP\n");
                continue;
            }

            // The server is receiving a connection request
            if (current_event.data.fd == server_fd)
            {
                while (true)
                {
                    client_fd = accept(server_fd, (struct sockaddr *)&remote_addr, &addr_len);
                    if (client_fd == -1)
                    {
                        if (errno != EAGAIN && errno != EWOULDBLOCK)
                        {
                            perror("accept");
                        }
                        continue;
                    }
                    // If we get here, then the client_fd is OK
                    break;
                }
                setSocketToNonBlock(&client_fd);

                // Add the client socket to the epoll instance
                event.data.fd = client_fd;
                event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
                status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                if (status == -1)
                {
                    perror("epoll_ctl");
                    exit(EXIT_FAILURE);
                }

                char host[NI_MAXHOST], serv[NI_MAXSERV];
                status = getnameinfo((struct sockaddr *)&remote_addr, addr_len, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
                if (status == -1)
                {
                    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(status));
                    continue;
                }
                printf("Connection from %s:%s\n", host, serv);
                continue;
            }
            if ((current_event.events & EPOLLIN) == EPOLLIN)
            {
                printf("Event on socket %d\n", current_event.data.fd);
                //                pthread_mutex_lock(&g_mutex);
                if (clearSocket(current_event.data.fd, buffer, bufLen))
                {
                    printf("Socket %d was cleared\n", current_event.data.fd);
                    //close(current_event.data.fd);
                }
                //                pthread_mutex_unlock(&g_mutex);
            }
            //        pthread_mutex_lock(ev_args->mutex);
            //if (!clearSocket(current_event.data.fd, buffer, bufLen)) { // One of the sockets has data to read
            //    close(current_event.data.fd);
            //}
            //        pthread_mutex_unlock(ev_args->mutex);
        }
    }

    if (events)
    {
        free(events);
    }
    if (buffer)
    {
        free(buffer);
    }
}

bool clearSocket(int socket, char *buf, const int len)
{
    // something is going wrong in this function...
    int bytesLeft = len;

    readAllFromSocket(socket, buf, len);

    bytesLeft = len;
    if (!sendToSocket(socket, buf, bytesLeft))
    {
        perror("sendall");
        fprintf(stderr, "Only %d bytes because of the error\n", bytesLeft);
    }
    return true;
    //while (true) {
    ////    bp = buf;
    //    while (n < len) {
    //        n = recv(socket, bp, bytesLeft, 0);
    //        if (errno == EAGAIN || n == 0) { // need this for edge-triggered mode
    //            return false;
    //        }
    //        bp += n;
    //        bytesLeft -= n;
    //    }
    //    printf("sending: %s\n", buf);
    //    n = send(socket, buf, len, 0);
    //    if (n == -1) {
    //        // client closed the connection
    //        return false;
    //    }
    //    return true;
    //}
    //close(socket);
    //return false;
}

void runEpoll(int listenSocket, const short port, const int bufferLength)
{
    int status;
    int server_fd;
    int epoll_fd;
    struct epoll_event event;
    event_loop_args args;

    server_fd = listenSocket;

    setSocketToNonBlock(&server_fd);

    epoll_fd = epoll_create1(0); // might need flags
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        return;
    }

    // Register the server socket for epoll events
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
    if (status == -1)
    {
        perror("epoll_ctl");
        return;
    }

    args.epoll_fd = epoll_fd;
    args.server_fd = server_fd;
    args.bufLen = (size_t)bufferLength;
    args.buffer = calloc(bufferLength, sizeof(char));

    //pthread_mutex_t* temp = NULL;
    //pthread_mutex_init(temp, NULL);

    //args.mutex = temp;

    // Start the event loop
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_create(workers + i, NULL, eventLoop, (void *)&args);
    }
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_join(workers[i], NULL);
    }
}

void epollSignalHandler(int sig)
{
    fprintf(stdout, "Stopping server\n");
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_cancel(workers[i]);
    }
}