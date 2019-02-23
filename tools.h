#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <sys/time.h>

void systemFatal(const char *message);

void formatTime(size_t *ms, size_t *us, const struct timeval *time);

int startLogging();
void stopLogging();

void logAcc(const int sock);
void logRcv(const int sock, const int amount);
void logSnd(const int sock, const int amount);

#endif // TOOLS_H