#include "util.h"
#include "multi-lookup.h"
#include <sys/time.h>
#include <pthread.h>

//helper function to "kickstart" the struct to be given to all threads
int initMultiLookupData(multiLookupData *data, array *array, int argc, char **argv, FILE *requestLog, FILE *resolveLog)
{
    if(!data || !array)
    {
        return -1;
    }
    data->sharedArray = array;
    data->argc = argc;
    data->argv = argv;
    data->fileIndex = 5;
    data->requestLog = requestLog;
    data->resolveLog = resolveLog;

    sem_init(&data->fileAccessSemaphore, 0, 1);
    sem_init(&data->printSemaphore, 0, 1);
    sem_init(&data->requestLogSemaphore, 0, 1);
    sem_init(&data->resolveLogSemaphore, 0, 1);

    return 0;
}

//main code for requester threads
void *requesterThread(void *arg)
{
    multiLookupData *data = (multiLookupData *)arg;
    int filesDone = 0;
    while(1) //keep looping forever until all files are accessed/read
    {
        sem_wait(&data->fileAccessSemaphore);
        if(data->fileIndex >= data->argc) //all files have been accessed
        {
            sem_post(&data->fileAccessSemaphore);
            break; //leave while loop
        }
        char *filename = data->argv[data->fileIndex];
        data->fileIndex = data->fileIndex + 1;
        sem_post(&data->fileAccessSemaphore);

        FILE *inputFile = fopen(filename, "r");
        if(inputFile == NULL) //just print an error and move on
        {
            sem_wait(&data->printSemaphore);
            fprintf(stderr, "invalid file %s\n", filename);
            sem_post(&data->printSemaphore);
        }
        else //actual requester stuff
        {
            char line[256];
            while(fgets(line, sizeof(line), inputFile) != NULL)
            {
                size_t length = strlen(line);
                if(length > 0 && line[length - 1] == '\n') //strip newline
                {
                    line[length - 1] = '\0';
                }
                
                array_put(data->sharedArray, line);

                sem_wait(&data->requestLogSemaphore);
                fprintf(data->requestLog, "%s\n", line);
                sem_post(&data->requestLogSemaphore);
            }
            fclose(inputFile);
            filesDone = filesDone + 1;
        }
    } 

    sem_wait(&data->printSemaphore);
    printf("thread %lu serviced %d files\n", (unsigned long)pthread_self(), filesDone);
    sem_post(&data->printSemaphore);

    return NULL;
}

//main code for resolver threads
void *resolverThread(void *arg)
{
    multiLookupData *data = (multiLookupData *)arg;
    char hostnameBuffer[MAX_NAME_LENGTH];
    char *hostname = hostnameBuffer;
    char ip[MAX_IP_LENGTH];
    int hostnamesDone = 0;
    while(1) //keep looping until poison pill is detected
    {
        array_get(data->sharedArray, &hostname);

        //poison pill detection
        if(strcmp(hostname, "...") == 0)
        {
            break; //we are done
        }

        //ip lookup
        int result = dnslookup(hostname, ip, sizeof(ip));
        if(result == 0)
        {
            sem_wait(&data->resolveLogSemaphore);
            fprintf(data->resolveLog, "%s, %s\n", hostname, ip);
            sem_post(&data->resolveLogSemaphore);
        }
        else
        {
            sem_wait(&data->resolveLogSemaphore);
            fprintf(data->resolveLog, "%s, NOT_RESOLVED\n", hostname);
            sem_post(&data->resolveLogSemaphore);
        }

        hostnamesDone = hostnamesDone + 1;
    }

    sem_wait(&data->printSemaphore);
    printf("thread %lu resolved %d hostnames\n", (unsigned long)pthread_self(), hostnamesDone);
    sem_post(&data->printSemaphore);

    return NULL;
}

int main(int argc, char *argv[])
{
    struct timeval start, end;
    if(gettimeofday(&start, NULL) == -1)
    {
        fprintf(stderr, "error: getting start time failed\n");
        exit(1);
    }

    //argument checks
    if(argc < 5)
    {
        printf("multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]\n");
        exit(1);
    }
    if((argc - 5) > MAX_INPUT_FILES)
    {
        fprintf(stderr, "error: too many input files\n");
        exit(1);
    }

    //extract request/resolve thread numbers
    int requesterNumber = atoi(argv[1]);
    int resolverNumber = atoi(argv[2]);
    if(requesterNumber < 1)
    {
        fprintf(stderr, "error: not enough requester threads, you need at least 1\n");
        exit(1);
    }
    if(requesterNumber > MAX_REQUESTER_THREADS)
    {
        fprintf(stderr, "error: too many requester threads, max is 10\n");
        exit(1);
    }
    if(resolverNumber < 1)
    {
        fprintf(stderr, "error: not enough resolver threads, you need at least 1\n");
        exit(1);
    }
    if(resolverNumber > MAX_RESOLVER_THREADS)
    {
        fprintf(stderr, "error: too many resolver threads, max is 10\n");
        exit(1);
    }

    //file opening stuff
    char *requestFilename = argv[3];
    char *resolveFilename = argv[4];
    FILE *requestLog = fopen(requestFilename, "w");
    if(requestLog == NULL)
    {
        fprintf(stderr, "error: failed to open, create, or wipe request file\n");
        exit(1);
    }
    FILE *resolveLog = fopen(resolveFilename, "w");
    if(resolveLog == NULL)
    {
        fprintf(stderr, "error: failed to open, create, or wipe resolve file\n");
        fclose(requestLog);
        exit(1);
    }

    //setting up array and struct
    multiLookupData data;
    array sharedArray;
    if(array_init(&sharedArray) != 0)
    {
        fprintf(stderr, "error: failed to start shared array\n");
        fclose(requestLog);
        fclose(resolveLog);
        exit(1);
    }
    if(initMultiLookupData(&data, &sharedArray, argc, argv, requestLog, resolveLog) != 0)
    {
        fprintf(stderr, "error: multilookup data struct failed to start\n");
        fclose(requestLog);
        fclose(resolveLog);
        array_free(&sharedArray);
        exit(1);
    }

    //setting up threads
    pthread_t requestThreads[requesterNumber];
    pthread_t resolveThreads[resolverNumber];
    for(int i = 0; i < requesterNumber; i++)
    {
        if(pthread_create(&requestThreads[i], NULL, requesterThread, &data) != 0)
        {
            fprintf(stderr, "error: error when starting a requester thread\n");
            fclose(requestLog);
            fclose(resolveLog);
            array_free(&sharedArray);
            sem_destroy(&data.fileAccessSemaphore);
            sem_destroy(&data.printSemaphore);
            sem_destroy(&data.requestLogSemaphore);
            sem_destroy(&data.resolveLogSemaphore);
            exit(1);
        }
    }
    for(int i = 0; i < resolverNumber; i++)
    {
        if(pthread_create(&resolveThreads[i], NULL, resolverThread, &data) != 0)
        {
            fprintf(stderr, "error: error when starting a resolver thread\n");
            fclose(requestLog);
            fclose(resolveLog);
            array_free(&sharedArray);
            sem_destroy(&data.fileAccessSemaphore);
            sem_destroy(&data.printSemaphore);
            sem_destroy(&data.requestLogSemaphore);
            sem_destroy(&data.resolveLogSemaphore);
            exit(1);
        }
    }

    //joining requesters to detect when all requester threads are done
    for(int i = 0; i < requesterNumber; i++)
    {
        pthread_join(requestThreads[i], NULL);
    }

    //add the poison pill to the shared array
    for(int i = 0; i < resolverNumber; i++)
    {
        array_put(data.sharedArray, "...");
    }

    //joining resolvers to detect when all resolver threads are done
    for(int i = 0; i < resolverNumber; i++)
    {
        pthread_join(resolveThreads[i], NULL);
    }

    //cleanup
    fclose(requestLog);
    fclose(resolveLog);
    sem_destroy(&data.fileAccessSemaphore);
    sem_destroy(&data.printSemaphore);
    sem_destroy(&data.requestLogSemaphore);
    sem_destroy(&data.resolveLogSemaphore);
    array_free(&sharedArray);

    //calculate runtime
    if(gettimeofday(&end, NULL) == -1)
    {
        fprintf(stderr, "error: getting end time failed\n");
        exit(1);
    }
    double totalRuntime = ((end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000000.0);
    printf("%s: total time is %f seconds\n", argv[0], totalRuntime);
    
    exit(0);
}