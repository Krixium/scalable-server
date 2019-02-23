#include "tools.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

FILE *logFile = NULL;
pthread_mutex_t lock;

void systemFatal(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void formatTime(size_t *ms, size_t *us, const struct timeval *time)
{
    *ms = time->tv_sec * 1000 + time->tv_usec / 1000;
    *us = time->tv_usec % 1000;
}

// return 1 if file was openned, 0 otherwise
int startLogging()
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

void stopLogging()
{
    pthread_mutex_lock(&lock);
    fflush(logFile);
    fclose(logFile);
    pthread_mutex_unlock(&lock);
}

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