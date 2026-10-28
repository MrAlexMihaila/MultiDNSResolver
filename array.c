#include "array.h"
#include <string.h>
#include <stdlib.h>

//initializes array
int array_init(array *s)
{
    s->head = 0;
    s->tail = 0;
    
    for(int i = 0; i < ARRAY_SIZE; i++)
    {
        s->array[i] = NULL;
    }

    sem_init(&s->mutex, 0, 1); //binary lock
    sem_init(&s->spaceInArray, 0, ARRAY_SIZE); //space in array, all slots open
    sem_init(&s->itemsInArray, 0, 0); //no items in array

    return 0;
}

//place element in array, block if full
int array_put(array *s, char *hostname)
{
    sem_wait(&s->spaceInArray); //wait until space in array exists
        sem_wait(&s->mutex);

        s->array[s->tail] = malloc(MAX_NAME_LENGTH * sizeof(char));
        if(s->array[s->tail] == NULL) //in case malloc fails
        {
            sem_post(&s->mutex);
            sem_post(&s->spaceInArray);
            return -1;
        }

        strncpy(s->array[s->tail], hostname, MAX_NAME_LENGTH - 1);
        s->array[s->tail][MAX_NAME_LENGTH - 1] = '\0'; //null terminator

        s->tail = (s->tail + 1) % ARRAY_SIZE; //sets tail from position 0 to array size - 1

        sem_post(&s->mutex);
    sem_post(&s->itemsInArray); //let other threads know that items exist in array
    return 0;
}

//remove element in array, block if empty
int array_get(array *s, char **hostname)
{
    sem_wait(&s->itemsInArray); //wait until items are in array
        sem_wait(&s->mutex);

        strncpy(*hostname, s->array[s->head], MAX_NAME_LENGTH - 1);
        (*hostname)[MAX_NAME_LENGTH - 1] = '\0'; //add the null terminator again

        free(s->array[s->head]);
        s->array[s->head] = NULL;

        s->head = (s->head + 1) % ARRAY_SIZE; //increments head by 1 and makes sure it's in range

        sem_post(&s->mutex);
    sem_post(&s->spaceInArray); //let other threads know that space is free in array
    return 0;
}

//free array resources by destroying all semaphores
void array_free(array *s)
{
    for(int i = 0; i < ARRAY_SIZE; i++) //clean up array before stopping
    {
        if(s->array[i] != NULL)
        {
            free(s->array[i]);
            s->array[i] = NULL;
        }
    }

    sem_destroy(&s->mutex);
    sem_destroy(&s->spaceInArray);
    sem_destroy(&s->itemsInArray);
}