/*---------------------------------------------------------------------------------------
-- SOURCE FILE:            tools.c
--
-- PROGRAM:                server.out
--
-- FUNCTIONS:
--                         void systemFatal(const char *message)
--                         void formatTime(size_t *ms, size_t *us, const struct timeval *time)
--                         bool startLogging()
--                         void stopLogging()
--                         void logAcc(const int sock)
--                         void logRcv(const int sock, const int amount)
--                         void logSnd(const int sock, const int amount)
--
-- DATE:                   Feb 19, 2019
--
-- REVISIONS:              N/A
--
-- DESIGNERS:              Benny Wang
--
-- PROGRAMMERS:            Benny Wang
--
-- NOTES:
-- General functions that are useful tools which aide in IO.
---------------------------------------------------------------------------------------*/
#include "tools.h"

#include <pthread.h>
#include <stdio.h>

FILE *logFile = NULL;
pthread_mutex_t lock;

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                systemFatal
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void systemFatal(const char *message)
--                              const char *message: The error message.
--
-- NOTES:
-- Prints out the error message and then exits the program with EXIT_FAILURE return value.
--------------------------------------------------------------------------------------------------*/
void systemFatal(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                formatTime
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void formatTime(size_t *ms, size_t *us, const struct timeval *time)
--                              size_t *ms: A pointer to hold the amount of milliseconds.
--                              size_t *us: A pointer to hold the amount of nanoseconds.
--                              const struct *time: A pointer to the time struct to convert.
--
-- NOTES:
-- Converts a timeval struct to milliseconds.
--------------------------------------------------------------------------------------------------*/
void formatTime(size_t *ms, size_t *us, const struct timeval *time)
{
    *ms = time->tv_sec * 1000 + time->tv_usec / 1000;
    *us = time->tv_usec % 1000;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                startLogging
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               int startLogging()
--
-- RETURNS:                 True if logging was started, false otherwise.
--
-- NOTES:
-- Opens the logging file.
--------------------------------------------------------------------------------------------------*/
bool startLogging()
{
    pthread_mutex_lock(&lock);
    logFile = fopen("server.log", "wb+");
    pthread_mutex_unlock(&lock);

    if (logFile == NULL)
    {
        return 0;
    }
    return 1;
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                stopLogging
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void stopLogging()
--
-- NOTES:
-- Flushs all writes to the log file and closes the log file.
--------------------------------------------------------------------------------------------------*/
void stopLogging()
{
    pthread_mutex_lock(&lock);
    fflush(logFile);
    fclose(logFile);
    pthread_mutex_unlock(&lock);
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                logAcc
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void logAcc(const int sock)
--                             const int sock: The socket that was accepted.
--
-- NOTES:
-- Logs an accept call to the log file. The format of the log is socket,timestamp(ms),new.
--------------------------------------------------------------------------------------------------*/
void logAcc(const int sock)
{
    size_t ms;
    size_t us;
    struct timeval timestamp;
    // try until it works
    while (gettimeofday(&timestamp, 0) == -1);
    formatTime(&ms, &us, &timestamp);
    pthread_mutex_lock(&lock);
    while (fprintf(logFile, "%d,%lu.%03lu,new\n", sock, ms, us) <= 0);
    pthread_mutex_unlock(&lock);
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                logRcv
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void logRcv(const int sock, const int amount)
--                             const int sock: The socket that was read from to.
--                             const int amount: The amount that was read from the socket.
--
-- NOTES:
-- Logs a recieve call to the log file. The format of the log is socket,timestamp(ms),rcv,amount(B).
--------------------------------------------------------------------------------------------------*/
void logRcv(const int sock, const int amount)
{
    size_t ms;
    size_t us;
    struct timeval timestamp;

    // try until it works
    while (gettimeofday(&timestamp, 0) == -1);
    formatTime(&ms, &us, &timestamp);
    pthread_mutex_lock(&lock);
    while (fprintf(logFile, "%d,%lu.%03lu,rcv,%d\n", sock, ms, us, amount) <= 0);
    pthread_mutex_unlock(&lock);
}

/*--------------------------------------------------------------------------------------------------
-- FUNCTION:                logSnd
--
-- DATE:                    Feb 19, 2019
--
-- REVISIONS:               N/A
--
-- DESIGNER:                Benny Wang
--
-- PROGRAMMER:              Benny Wang
--
-- INTERFACE:               void logSnd(const int sock, const int amount)
--                             const int sock: The socket that was written to.
--                             const int amount: The amount that was written to the socket.
--
-- NOTES:
-- Logs a send call to the log file. The format of the log is socket,timestamp(ms),snd,amount(B).
--------------------------------------------------------------------------------------------------*/
void logSnd(const int sock, const int amount)
{
    size_t ms;
    size_t us;
    struct timeval timestamp;

    // try until it works
    while (gettimeofday(&timestamp, 0) == -1);
    formatTime(&ms, &us, &timestamp);
    pthread_mutex_lock(&lock);
    while (fprintf(logFile, "%d,%lu.%03lu,snd,%d\n", sock, ms, us, amount) <= 0);
    pthread_mutex_unlock(&lock);
}