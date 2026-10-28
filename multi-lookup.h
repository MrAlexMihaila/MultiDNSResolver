#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include "array.h"
#include <stdio.h>
#include <netinet/in.h>
#include <semaphore.h>

#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

typedef struct {
    array *sharedArray;

    int argc;
    char **argv;
    int fileIndex;

    FILE *requestLog;
    FILE *resolveLog;

    sem_t fileAccessSemaphore; //request threads only, manages accessing argc/argv
    sem_t printSemaphore; //both threads, manages printing to console
    sem_t requestLogSemaphore; //request threads only, manages writing to request log
    sem_t resolveLogSemaphore; //resolve threads only, manages writing to resolve log
} multiLookupData;

int initMultiLookupData(multiLookupData *data, array *array, int argc, char **argv, FILE *requestLog, FILE *resolveLog); //handles starting struct
void *requesterThread(void *arg);
void *resolverThread(void *arg);

#endif