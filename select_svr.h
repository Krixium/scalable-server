#ifndef SELECT_SVR_H
#define SELECT_SVR_H

#include <pthread.h>

void runSelect(int listenSocket, pthread_t *workers, const int numWorkers, const short port, const int bufferLength);

void *selectWorker(void *args);

#endif // SELECT_SVR_H