#include "main.h"

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <unistd.h>

#include "net.h"
#include "tools.h"
#include "select_svr.h"
#include "epoll_svr.h"

#define SELECT_MODE 1
#define EPOLL_MODE 2

short port;
int mode;
int bufferLength;

int main(int argc, char *argv[])
{
    int listenSocket;

    // grab arguements
    parseArguments(argc, argv);

    // open logging file
    if (!startLogging())
    {
        perror("Could not create log file\n");
        exit(1);
    }

    // create listening socket
    if (!createBoundSocket(&listenSocket, port))
    {
        perror("Could not bind a socket");
        exit(1);
    }

    listen(listenSocket, 5);

    // pick mode
    switch (mode)
    {
    case SELECT_MODE:
        runSelect(listenSocket, port, bufferLength);
        break;
    case EPOLL_MODE:
        runEpoll(listenSocket, port, bufferLength);
        break;
    default:
        printHelp(argv[0]);
        exit(1);
    }

    // close the listen socket
    close(listenSocket);

    // close logging file
    stopLogging();

    return 0;
}

void parseArguments(int argc, char *argv[])
{
    int c;

    mode = 0;
    port = 0;
    bufferLength = 0;

    while ((c = getopt(argc, argv, "m:p:b:")) != -1)
    {
        switch (c)
        {
        case 'm':
            if (!strcmp(optarg, "select"))
            {
                mode = SELECT_MODE;
            }
            else if (!strcmp(optarg, "epoll"))
            {
                mode = EPOLL_MODE;
            }
            else
            {
                printHelp(argv[0]);
                exit(1);
            }
            break;
        case 'p':
            port = atoi(optarg);
            if (port < 1024)
            {
                fprintf(stderr, "Port must be higher than 1024\n");
                exit(1);
            }
            break;
        case 'b':
            bufferLength = atoi(optarg);
            break;
        default:
            printHelp(argv[0]);
            exit(1);
        }
    }

    // check that all options have been given
    if (!mode || !port || !bufferLength)
    {
        printHelp(argv[0]);
        exit(1);
    }
}

void printHelp(const char *name)
{
    fprintf(stderr, "Usage: %s -m [select|epoll] -p [port] -b [buffer size]\n", name);
    fprintf(stderr, "    -m - The operatin mode. Either 'select' or 'epoll.'\n");
    fprintf(stderr, "    -p - The port to listen on. Must be greater than 1024.\n");
    fprintf(stderr, "    -b - The buffer size. Recommendation is less than 1000.\n");
}
