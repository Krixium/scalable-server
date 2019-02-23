#include "net.h"

#include <fcntl.h>
#include <strings.h>
#include <unistd.h>

#include "tools.h"

// returns true on success, false otherwise
bool setSocketToReuse(int sock)
{
    const int arg = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(int)) == -1)
    {
        return false;
    }

    return true;
}

// returns true on success, false otherwise
bool setSocketToNonBlocking(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        return false;
    }

    return true;
}

// returns true on success, false otherwise
bool createTCPSocket(int *sock)
{
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        return false;
    }

    return setSocketToReuse(*sock);
}

// returns true on success, false otherwise
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

// returns true on success, false otherwise
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

// returns the number of bytes read
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

    logRcv(sock, size - remaining);

    return size - remaining;
}

// returns number of bytes written
int sendToSocket(const int sock, char *buffer, const int size)
{
    int n = send(sock, buffer, size, 0);

    logSnd(sock, n);

    return n;
}

// returns true if socket was cleared, false otherwise and closes the socket
bool clearSocket(int sock, char *buf, const int len)
{
    int nRead = readAllFromSocket(sock, buf, len);

    if (nRead <= 0)
    {
        close(sock);
        return false;
    }

    if (!sendToSocket(sock, buf, len))
    {
        close(sock);
        return false;
    }

    return true;
}
