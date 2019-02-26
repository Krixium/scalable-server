/*---------------------------------------------------------------------------------------
-- SOURCE FILE:            net.c
--
-- PROGRAM:                server.out
--
-- FUNCTIONS:
--                         bool setSocketToReuse(int sock)
--                         bool setSocketToNonBlocking(int sock)
--                         bool setSocketTimeout(const size_t sec, const size_t usec, const int sock)
--                         bool createTCPSocket(int *sock)
--                         bool createBoundSocket(int *sock, const short port)
--                         bool acceptNewConnection(const int listenSocket, int *newSocket, struct sockaddr_in *client)
--                         int readAllFromSocket(const int sock, char *buffer, const int size)
--                         int sendToSocket(const int sock, char *buffer, const int size)
--                         bool clearSocket(int socket, char* buf, const int len)
--
-- DATE:                   Feb 19, 2019
--
-- REVISIONS:              N/A
--
-- DESIGNERS:              Benny Wang, William Murphy
--
-- PROGRAMMERS:            Benny Wang, Wiliam Murphy
--
-- NOTES:
-- Contains wrapper functions for network related system calls.
---------------------------------------------------------------------------------------*/
#include "net.h"

#include <fcntl.h>
#include <strings.h>
#include <unistd.h>

#include "tools.h"

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                setSocketToReuse
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               bool setSocketToReuse(int sock)
--                              int sock: The socket to configure.
--
-- RETURNS:                 True if the socket reuse flag was set, false otherwise.
--
-- NOTES:
-- Sets the socket's reuse flag so that the socket can be reused in a crash scenario.
--------------------------------------------------------------------------------------------------*/
bool setSocketToReuse(int sock)
{
    const int arg = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(int)) == -1)
    {
        return false;
    }

    return true;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                setSocketToNonBlocking
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               bool setSocketToNonBlocking(int sock)
--                              int sock: The socket to configure.
--
-- RETURNS:                 True if the socket was configured, false otherwise.
--
-- NOTES:
-- Sets the socket to non-blocking.
--------------------------------------------------------------------------------------------------*/
bool setSocketToNonBlocking(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        return false;
    }

    return true;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                setSocketTimeout
--
-- DATE:                    February 11, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               int setSocketTimeout
--                              const size_t sec: The amount of seconds to wait before timing out.
--                              const size_t sec: The amount of nanosecond to wait before timing out.
--
-- RETURN:                  1 if the socket options were set, 0 otherwise.
--
-- NOTES:
--                          Sets the receive timeout and send timeout of a socket.
--------------------------------------------------------------------------------------------------*/
bool setSocketTimeout(const size_t sec, const size_t usec, const int sock)
{
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        return false;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        return false;
    }

    return true;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                createTCPSocket
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               bool createTCPSocket(int *sock)
--                              int *sock: A pointer to hold the new socket.
--
-- RETURNS:                 True if a TCP socket was created, false otherwise.
--
-- NOTES:
-- Creates a TCP socket and sets the reuse flag of the socket.
--------------------------------------------------------------------------------------------------*/
bool createTCPSocket(int *sock)
{
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        return false;
    }

    return setSocketToReuse(*sock);
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                createBoundSocket
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               bool createBoundSocket(int *sock, const short port)
--                              int *sock: A pointer to hold the new socket.
--                              const short port: The port to bind on.
--
-- RETURNS:                 True of a bound TCP socket was created, false otherwise.
--
-- NOTES:
-- Creates a bound TCP socket on port port and places it into sock. The socket will be bound for
-- address INADDR_ANY.
--------------------------------------------------------------------------------------------------*/
bool createBoundSocket(int *sock, const short port)
{
    struct sockaddr_in server;

    if (!createTCPSocket(sock))
    {
        return false;
    }

    bzero(&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(*sock, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        return false;
    }

    return true;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                acceptNewConnection
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               bool acceptNewConnection(const int listenSocket, int *newSocket, struct sockaddr_in *client)
--                              const int listenSocket: The listening socket.
--                              int *newSocket: A pointer hold the new socket.
--                              struct sockaddr_in *client: A pointer to hold the new client.
--
-- RETURNS:                 True if the client was accepted, false otherwise.
--
-- NOTES:
-- Accepts a new connection and places the new socket into newSocket and new sockaddr_in of the new
-- client into client. Must call startLogging() located in tools.h once before executing this funciton.
--------------------------------------------------------------------------------------------------*/
bool acceptNewConnection(const int listenSocket, int *newSocket, struct sockaddr_in *client)
{
    unsigned int length = sizeof(struct sockaddr_in);
    bzero(client, length);
    if ((*newSocket = accept(listenSocket, (struct sockaddr *)client, &length)) == -1)
    {
        return false;
    }

    logAcc(*newSocket);

    return true;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                readAllFromSocket
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               int readAllFromSocket(const int sock, char *buffer, const int size)
--                              const int sock: The socket to read.
--                              char *buffer: The buffer for read data.
--                              const int size: The size of the buffer.
--
-- RETURNS:                 The number of bytes read.
--
-- NOTES:
-- Attempts to read size bytes from sock into buffer and then logs the amount of data read. Must
-- call startLogging() located in tools.h once before executing this function.
--------------------------------------------------------------------------------------------------*/
int readAllFromSocket(const int sock, char *buffer, const int size)
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

    if (size - remaining > 0)
    {
        logRcv(sock, size - remaining);
    }

    return size - remaining;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                sendToSocket
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               int sendToSocket(const int sock, char *buffer, const int size)
--                              const int sock: The socket to write.
--                              char *buffer: The buffer to send.
--                              const int size: The length of the buffer.
--
-- RETURNS:                 The number of bytes sent.
--
-- NOTES:
-- Sends the contents of buffer to sock and logs the amount of data that was sent. Must call
-- startLogging() located in tools.h once before executing this function.
--------------------------------------------------------------------------------------------------*/
int sendToSocket(const int sock, char *buffer, const int size)
{
    if (size <= 0)
    {
        return 0;
    }
    int n = send(sock, buffer, size, 0);
    if (n > 0)
    {
        logSnd(sock, n);
    }
    return n;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                clearSocket
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                William Murphy
--
-- PROGRAMMER:              William Murphy
--
-- INTERFACE:               bool clearSocket(int sock, char *buf, const int len)
--                              int sock: The socket to read.
--                              char *buf: The buffer to place the data.
--                              const int len: The length of the buffer.
--
-- RETURNS:                 The number of bytes received, -1 if receiving failed.
--
-- NOTES:
-- Reads all the data from the sock into buf and then sends buf to sock.
--------------------------------------------------------------------------------------------------*/
int clearSocket(int sock, char *buf, const int len)
{
    int n = readAllFromSocket(sock, buf, len);
    sendToSocket(sock, buf, n);
    return n;
}
