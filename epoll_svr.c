#include "epoll_svr.h"

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

static const int MAX_EVENTS = 256;
//static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static int readAllFromSocket(const int sock, char *buffer, const int size)
{
    int n = 1;
    char *bufferPointer = buffer;
    int remaining = size;

    while (n > 0 && remaining > 0)
    {
        n = recv(sock, bufferPointer, remaining, 0);
        if (errno == EAGAIN) {
            break;
        }
        bufferPointer += n;
        remaining -= n;
    }

//    logRcv(sock, size - remaining);

    return size - remaining;
}

int setNonBlocking(int fd) {
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}

typedef struct {
	int nClients;
	int epoll_fd;
	int server_fd;
	int bufLen;
	char* buffer;
	struct epoll_event* events;
	pthread_mutex_t* mutex;
} event_loop_args;

void* eventLoop(void* args);
bool clearSocket(int socket, char* buf, const int len);

bool recvAll(int s, char* buf, int* len);
bool sendAll(int s, char* buf, int* len);


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

bool clearSocket(int socket, char* buf, const int len) {

    int bytesLeft = len;
	int nRead = readAllFromSocket(socket, buf, len);
	printf("Read %d bytes from socket %d\n", nRead, socket);
	printf("Buffer after recv: %s\n", buf);
	if (!sendAll(socket, buf, &bytesLeft)) {
		perror("sendall");
		fprintf(stderr, "Only %d bytes because of the error\n", bytesLeft);
	}
	//close(socket);
	return true;
}

void runEpoll(int listenSocket, pthread_t *workers, const int numWorkers, const short port, const int bufferLength)
{
	int status;
	struct epoll_event event;
	event_loop_args* args = calloc(1, sizeof(event_loop_args));

	args->server_fd = listenSocket;

	setNonBlocking(args->server_fd);

	args->epoll_fd = epoll_create1(0); // might need flags
	if (args->epoll_fd == -1) {
		perror("epoll_create1");
		return;
	}

	// Register the server socket for epoll events
	event.data.fd = args->server_fd;
	event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
	status = epoll_ctl(args->epoll_fd, EPOLL_CTL_ADD, args->server_fd, &event);
	if (status == -1) {
		perror("epoll_ctl");
		return;
	}


	args->bufLen = (size_t) bufferLength;
	args->buffer = calloc(bufferLength, sizeof(char));
	args->events = calloc(MAX_EVENTS, sizeof(struct epoll_event));

	//pthread_mutex_t* temp = NULL;
	//pthread_mutex_init(temp, NULL);

	//args.mutex = temp;

	// Start the event loop
    for (int i = 0; i < numWorkers; i++)
    {
        pthread_create(workers + i, NULL, eventLoop, (void*) args);
    }
}

bool sendAll(int s, char* buf, int* len) {
	int total = 0;
	int bytesLeft = *len;
	int n;

	while (total < *len) {
		n = send(s, buf + total, bytesLeft, 0);
		if (n == -1) {
			return false;
		}
		total += n;
		bytesLeft -= n;
	}

	*len = total;

	return n == -1;
}

bool recvAll(int s, char* buf, int* len) {
	return true;
}
