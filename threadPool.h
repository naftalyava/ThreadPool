#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "osqueue.h"

typedef struct thread_pool_task
{
    void (*computeFunc)(void *);
    void *param;
} ThreadPoolTask;

typedef enum thread_pool_state
{
    RUNNING = 0,
    SOFT_SHUTDOWN = 1,
    HARD_SHUTDOWN = 2
} ThreadPoolState;

typedef struct thread_pool
{
    /* queue */
    OSQueue* queue;
    unsigned int queueSize;

    /* pool */
    ThreadPoolState state;
    unsigned int poolSize;
    
    
    /* pthread */
    pthread_t* threads;
    pthread_mutex_t lock;
    pthread_cond_t cnd;
} ThreadPool;


ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
