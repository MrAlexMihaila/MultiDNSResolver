#ifndef ARRAY_H
#define ARRAY_H

#include <semaphore.h>

#define ARRAY_SIZE 8
#define MAX_NAME_LENGTH 256

typedef struct {
    char *array[ARRAY_SIZE];
    int head; //position of first element in the fifo array
    int tail; //position of last element in the fifo array
    sem_t mutex; //only one thread can access data at a time
    sem_t spaceInArray; //semaphore to track how much space is in the array
    sem_t itemsInArray; //semaphore to track how many items are in the array
} array; //this will be a circular queue

int array_init(array *s); //initializes array
int array_put(array *s, char *hostname); //place element in array, block if full
int array_get(array *s, char **hostname); //remove element in array, block if empty
void array_free(array *s); //free array resources

#endif