#include "tools.h"

FILE *logFile = NULL;

void formatTime(size_t *ms, size_t *us, const struct timeval *time)
{
    *ms = time->tv_sec * 1000 + time->tv_usec / 1000;
    *us = time->tv_usec % 1000;
}

// return 1 if file was openned, 0 otherwise
int startLogging()
{
    logFile = fopen("server.log", "wb+");
    if (logFile == NULL)
    {
        return 0;
    }
    
    return 1;
}

void stopLogging()
{
    fflush(logFile);
    fclose(logFile);
}

void logAcc(const int sock)
{
    size_t ms;
    size_t us;
    struct timeval timestamp;

    while (gettimeofday(&timestamp, 0) == -1);
    formatTime(&ms, &us, &timestamp);
    while (fprintf(logFile, "%d,%lu.%03lu,new\n", sock, ms, us));
}

void logRcv(const int sock, const int amount)
{
    size_t ms;
    size_t us;
    struct timeval timestamp;

    // try until it works
    while (gettimeofday(&timestamp, 0) == -1);
    formatTime(&ms, &us, &timestamp);
    while (fprintf(logFile, "%d,%lu.%03lu,rcv,%d\n", sock, ms, us, amount) <= 0);
}

void logSnd(const int sock, const int amount)
{
    size_t ms;
    size_t us;
    struct timeval timestamp;

    // try until it works
    while (gettimeofday(&timestamp, 0) == -1);
    formatTime(&ms, &us, &timestamp);
    while (fprintf(logFile, "%d,%lu.%03lu,snd,%d\n", sock, ms, us, amount) <= 0);
}