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

//static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static int readAllFromSocket(const int sock, char *buffer, const int size)
{
    int n = 1;
    char *bufferPointer = buffer;
    int remaining = size;

    while (n > 0 && remaining > 0)
    {
        n = recv(sock, bufferPointer, remaining, 0);
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
	pthread_mutex_t* mutex;
} event_loop_args;

void* eventLoop(void* args);
bool clearSocket(int socket, char* buf, const int len);

bool recvAll(int s, char* buf, int* len);
bool sendAll(int s, char* buf, int* len);


void* eventLoop(void* args) {
	const int MAX_EVENTS = 256;

	struct epoll_event current_event, event;
	int n_ready;
	int status;
	struct sockaddr_storage remote_addr;
	socklen_t addr_len;

	event_loop_args* ev_args = (event_loop_args*) args;

	int server_fd = ev_args->server_fd;
	int epoll_fd = ev_args->epoll_fd;
	const int bufLen = ev_args->bufLen;

	char* buffer = ev_args->buffer;
	struct epoll_event *events = calloc(MAX_EVENTS, sizeof(struct epoll_event));

	while (true) {
		n_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (n_ready == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		// A file descriptor is ready
		for (int i = 0; i < n_ready; ++i) {
			int client_fd;
			current_event = events[i];
			// An error or hangup occurred
			// Client might have closed their side of the connection
			if (current_event.events & (EPOLLHUP | EPOLLERR)) {
				fprintf(stderr, "epoll: EPOLLERR or EPOLLHUP\n");
				continue;
			}

			// The server is receiving a connection request
			if (current_event.data.fd == server_fd) {
				while (true) {
					client_fd = accept(server_fd, (struct sockaddr*) &remote_addr, &addr_len);
					if (client_fd == -1) {
						if (errno != EAGAIN && errno != EWOULDBLOCK) {
							perror("accept");
						}
						continue;
					}
					// If we get here, then the client_fd is OK
					break;
				}
				setNonBlocking(client_fd);

				// Add the client socket to the epoll instance
				event.data.fd = client_fd;
				event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
				status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
				if (status == -1) {
					perror("epoll_ctl");
					exit(EXIT_FAILURE);
				}

				char host[NI_MAXHOST], serv[NI_MAXSERV];
				status = getnameinfo((struct sockaddr*) &remote_addr, addr_len, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
				if (status == -1) {
					fprintf(stderr, "getnameinfo: %s\n", gai_strerror(status));
					continue;
				}
				printf("Connection from %s:%s\n", host, serv);
				continue;
			} 
			if ((current_event.events & EPOLLIN) == EPOLLIN) {
				printf("Event on socket %d\n", current_event.data.fd);
//				pthread_mutex_lock(&g_mutex);
				if (clearSocket(current_event.data.fd, buffer, bufLen)) {
					printf("Socket %d was cleared\n", current_event.data.fd);
					//close(current_event.data.fd);
				}
//				pthread_mutex_unlock(&g_mutex);
			}
	//		pthread_mutex_lock(ev_args->mutex);
			//if (!clearSocket(current_event.data.fd, buffer, bufLen)) { // One of the sockets has data to read
			//	close(current_event.data.fd);
			//}
	//		pthread_mutex_unlock(ev_args->mutex);
		}
	}

	if (events) {
		free(events);
	}
	if (buffer) {
		free(buffer);
	}
}

bool clearSocket(int socket, char* buf, const int len) {
	// something is going wrong in this function...
	int bytesLeft = len;

	readAllFromSocket(socket, buf, len);

	bytesLeft = len;
	if (!sendAll(socket, buf, &bytesLeft)) {
		perror("sendall");
		fprintf(stderr, "Only %d bytes because of the error\n", bytesLeft);
	}
	return true;
	//while (true) {
	////	bp = buf;
	//	while (n < len) {
	//		n = recv(socket, bp, bytesLeft, 0);
	//		if (errno == EAGAIN || n == 0) { // need this for edge-triggered mode
	//			return false;
	//		}
	//		bp += n;
	//		bytesLeft -= n;
	//	}
	//	printf("sending: %s\n", buf);
	//	n = send(socket, buf, len, 0);
	//	if (n == -1) {
	//		// client closed the connection
	//		return false;
	//	}
	//	return true;
	//}
	//close(socket);
	//return false;
}

void runEpoll(int listenSocket, pthread_t *workers, const int numWorkers, const short port, const int bufferLength)
{
	int status;
	int server_fd;
	int epoll_fd;
	struct epoll_event event;
	event_loop_args args;

	server_fd = listenSocket;

	setNonBlocking(server_fd);

	epoll_fd = epoll_create1(0); // might need flags
	if (epoll_fd == -1) {
		perror("epoll_create1");
		return;
	}

	// Register the server socket for epoll events
	event.data.fd = server_fd;
	event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
	status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
	if (status == -1) {
		perror("epoll_ctl");
		return;
	}


	args.epoll_fd = epoll_fd;
	args.server_fd = server_fd;
	args.bufLen = (size_t) bufferLength;
	args.buffer = calloc(bufferLength, sizeof(char));

	//pthread_mutex_t* temp = NULL;
	//pthread_mutex_init(temp, NULL);

	//args.mutex = temp;

	// Start the event loop
    for (int i = 0; i < numWorkers; i++)
    {
        pthread_create(workers + i, NULL, eventLoop, (void*) &args);
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
