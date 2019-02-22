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

// Globals
static const int MAX_EVENTS = 256;
pthread_t* workers;

bool clearSocket(int socket, char* buf, const int len);

bool recvAll(int s, char* buf, int* len);
bool sendAll(int s, char* buf, int* len);


static int setNonBlocking(int fd) 
{
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}

void* eventLoop(void* args) {

	struct epoll_event current_event, event;
	int n_ready;
	int status;
	struct sockaddr_storage remote_addr;
	socklen_t addr_len;
	char* local_buffer;

	event_loop_args* ev_args = (event_loop_args*) args;

	while (true) {
		n_ready = epoll_wait(ev_args->epoll_fd, ev_args->events, MAX_EVENTS, -1);
		if (n_ready == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		// A file descriptor is ready
		for (int i = 0; i < n_ready; ++i) {
			int client_fd;
			current_event = ev_args->events[i];
			// An error or hangup occurred
			// Client might have closed their side of the connection
			if (current_event.events & (EPOLLHUP | EPOLLERR)) {
				fprintf(stderr, "epoll: EPOLLERR or EPOLLHUP\n");
				continue;
			}

			// The server is receiving a connection request
			if (current_event.data.fd == ev_args->server_fd) {
				while (true) {
					client_fd = accept(ev_args->server_fd, (struct sockaddr*) &remote_addr, &addr_len);
					if (client_fd == -1) {
						if (errno != EAGAIN && errno != EWOULDBLOCK) {
							perror("accept");
						}
						continue;
					}
				    char host[NI_MAXHOST], serv[NI_MAXSERV];
				    status = getnameinfo((struct sockaddr*) &remote_addr, addr_len, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
				    if (status == -1) {
					    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(status));
					    continue;
				    }
				    printf("Connection from %s:%s\n", host, serv);
					// If we get here, then the client_fd is OK
					break;
				}
				setNonBlocking(client_fd);

				// Add the client socket to the epoll instance
				event.data.fd = client_fd;
				event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
				status = epoll_ctl(ev_args->epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
				if (status == -1) {
					perror("epoll_ctl");
					exit(EXIT_FAILURE);
				}

				continue;
			}
			if ((current_event.events & EPOLLIN) == EPOLLIN) {
				printf("Event on socket %d\n", current_event.data.fd);
				local_buffer = calloc(ev_args->bufLen, sizeof(char));
				if (clearSocket(current_event.data.fd, local_buffer, ev_args->bufLen)) {
					printf("Socket %d was cleared\n", current_event.data.fd);
					//close(current_event.data.fd);
					free(local_buffer);
				}
			}
		}
	}
}

bool clearSocket(int socket, char *buf, const int len)
{
    
    int bytesLeft = len;

    readAllFromSocket(socket, buf, len);

    bytesLeft = len;
    if (!sendToSocket(socket, buf, bytesLeft))
    {
        perror("sendall");
        fprintf(stderr, "Only %d bytes because of the error\n", bytesLeft);
    }
    return true;
}

void runEpoll(int listenSocket, const short port, const int bufferLength)
{
    int status;
    struct epoll_event event;
    event_loop_args* args = calloc(1, sizeof(event_loop_args));

    args->server_fd = listenSocket;

    setNonBlocking(args->server_fd);

    args->epoll_fd = epoll_create1(0); // might need flags
    if (args->epoll_fd == -1) 
    {
      perror("epoll_create1");
      return;
    }

    // Register the server socket for epoll events
    event.data.fd = args->server_fd;
    event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    status = epoll_ctl(args->epoll_fd, EPOLL_CTL_ADD, args->server_fd, &event);
    if (status == -1) 
    {
      perror("epoll_ctl");
      return;
    }

    args->bufLen = (size_t)bufferLength;
    args->buffer = calloc(bufferLength, sizeof(char));
    args->events = calloc(MAX_EVENTS, sizeof(struct epoll_event));

    //pthread_mutex_t* temp = NULL;
    //pthread_mutex_init(temp, NULL);

    //args.mutex = temp;

    workers = calloc(get_nprocs(), sizeof(pthread_t));

    // Start the event loop
    for (int i = 0; i < get_nprocs(); i++)
    {
        pthread_create(workers + i, NULL, eventLoop, (void*) args);
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
