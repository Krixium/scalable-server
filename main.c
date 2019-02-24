/*---------------------------------------------------------------------------------------
-- SOURCE FILE:            main.c
--
-- PROGRAM:                server.out
--
-- FUNCTIONS:
--                         parseArguments(int argc, char *argv[])
--                         printHelp(const char *name)
--                         signalHandler(int sig)
--
-- DATE:                   Feb 19, 2019
--
-- REVISIONS:              N/A
--
-- DESIGNERS:              Benny Wang, William Murphy
--
-- PROGRAMMERS:            Benny Wang
--
-- NOTES:
-- The main entry point the program. Parses the command line arguments and then if all
-- the argements are okay, starts the server in either epoll or select mode.
-- 
-- For usage see the printHelp() function or README.md file.
---------------------------------------------------------------------------------------*/
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

/*---------------------------------------------------------------------------------------
-- FUNCTION:                main
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               int main(int argc, char *argv[])
--                              int argc: The number of command line arguments.
--                              char *argv[]: The command line arguments.
--
-- RETURNS:                 The exit code of the program.
--
-- NOTES:
-- The main entry point of the program.
---------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    int listenSocket;

    // grab arguements
    parseArguments(argc, argv);

    // open logging file
    if (!startLogging())
    {
        systemFatal("startLogging");
    }

    // create listening socket
    if (!createBoundSocket(&listenSocket, port))
    {
        systemFatal("createBoundSocket");
    }

    listen(listenSocket, 5);

    // pick mode
    switch (mode)
    {
    case SELECT_MODE:
        runSelect(listenSocket, bufferLength);
        break;
    case EPOLL_MODE:
        runEpoll(listenSocket, bufferLength);
        break;
    default:
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }

    // close the listen socket
    close(listenSocket);

    // close logging file
    stopLogging();

    return 0;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                parseArguments
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void parseArguments(int argc, char *argv[])
--                              int argc: Argc from main.
--                              char *argv[]: Argv from main.
--
-- NOTES:
-- Parses the command line arguments and configures the approriate settings.
--------------------------------------------------------------------------------------------------*/
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
                exit(EXIT_FAILURE);
            }
            break;
        case 'b':
            bufferLength = atoi(optarg);
            break;
        default:
            printHelp(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // check that all options have been given
    if (!mode || !port || !bufferLength)
    {
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                printHelp
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void printHelp(const char *name)
--                              name: The name of the application.
--
-- NOTES:
-- Prints the help menu with instructions on how to run the program.
--------------------------------------------------------------------------------------------------*/
void printHelp(const char *name)
{
    fprintf(stderr, "Usage: %s -m [select|epoll] -p [port] -b [buffer size]\n", name);
    fprintf(stderr, "    -m - The operatin mode. Either 'select' or 'epoll.'\n");
    fprintf(stderr, "    -p - The port to listen on. Must be greater than 1024.\n");
    fprintf(stderr, "    -b - The buffer size. Recommendation is less than 1000.\n");
}
