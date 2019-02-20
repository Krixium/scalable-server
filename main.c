#include "main.h"

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <pthread.h>

#include "net.h"
#include "tools.h"
#include "select_svr.h"
#include "epoll_svr.h"

#define SELECT_MODE 1
#define EPOLL_MODE  2

short port;
int bufferLength;
int mode;

int workerCount;
pthread_t *workers;

int main(int argc, char *argv[])
{
    int listenSocket;

    // override ^c
    signal(SIGINT, signalHandler);

    // grab arguements
    parseArguments(argc, argv);

    // open logging file
    if (!startLogging())
    {
        perror("Could not create log file\n");
        exit(1);
    }

    // initialize worker array
    workerCount = get_nprocs();
    if ((workers = calloc(sizeof(pthread_t), workerCount)) == NULL)
    {
        perror("Could not allocate space for threads");
        exit(1);
    }

    // create listening socket
    if (!createBoundSocket(&listenSocket, port))
    {
        perror("Could not bind a socket");
        exit(1);
    }
    listen(listenSocket, 10);

    // pick mode
    switch (mode)
    {
    case SELECT_MODE:
        runSelect(listenSocket, workers, workerCount, port, bufferLength);
        break;
    case EPOLL_MODE:
        runEpoll(listenSocket, workers, workerCount, port, bufferLength);
        break;
    default:
        printHelp(argv[0]);
        exit(1);
    }

    // wait for workers to finish
    for (int i = 0; i < workerCount; i++)
    {
        pthread_join(workers[i], 0);
    }

    // close the listen socket
    close(listenSocket);

    // free worker array
    free(workers);

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

void signalHandler(int sig)
{
    fprintf(stdout, "Stopping server\n");
    for (int i = 0; i < workerCount; i++)
    {
        pthread_cancel(workers[i]);
    }
}