#include "threadPool.h"

/*  Data structure */




/*  Internal methods */

static void* threadPoolThread(void* tp){
    ThreadPool* pool = (ThreadPool*)(tp);
    for(;;) 
    {
        pthread_mutex_lock(&(pool->lock));

        /* Handle waiting for work RUNNING and SOFT_SHUTDOWN */
        while ((pool->state == RUNNING) && (pool->queueSize == 0))
        {
            pthread_cond_wait(&(pool->cnd), &(pool->lock));
        }
        
        /* Handle SOFT_SHUTDOWN termination */
        if ((pool->queueSize == 0) && (pool->state != RUNNING))
        {   
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL); 
            return(NULL);
        }

        /* Handle HARD_SHUTDOWN termination */
        if (pool->state == HARD_SHUTDOWN)
        {   
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
            return(NULL);
        }

        /* Get and execute work */
        ThreadPoolTask* task = osDequeue(pool->queue);
        pool->queueSize--;
        pthread_mutex_unlock(&(pool->lock));

        (*(task->computeFunc))(task->param);
        free(task);
    } 
}


ThreadPool* tpCreate(int numOfThreads)
{
	ThreadPool* threadPool = malloc(sizeof(ThreadPool));
	if(threadPool == NULL) return NULL;


	/* Initialize pool data*/
	threadPool->state = RUNNING;
	threadPool->poolSize = numOfThreads;

    /* Initialize queue data*/
    threadPool->queueSize = 0;
    threadPool->queue = osCreateQueue();
    if (threadPool->queue == NULL)
    {

    }

    /* Initialize pthreads */
    pthread_mutex_init(&(threadPool->lock), NULL);
    pthread_cond_init(&(threadPool->cnd), NULL);
	threadPool->threads = malloc(sizeof(pthread_t) * numOfThreads);
	if (threadPool->threads == NULL)
	{
        free(threadPool);
        return NULL;
	}
    int i, liveThreads = 0; 
    for (i=0; i < numOfThreads; i++)
    {
        threadPool->threads[i] = malloc(sizeof(pthread_t));
    }

    /* Start worker threads */
    for(i = 0; i < threadPool->poolSize; i++) 
    {
        pthread_create(&(threadPool->threads[i]), NULL, threadPoolThread, threadPool);
        liveThreads++;
    }

    /* Wait for worker threads to initialize */
    while (liveThreads < threadPool->poolSize) {}

    return threadPool;
}



void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks)
{
    pthread_mutex_lock(&(threadPool->lock));

    /* Hand repeated call fr tpDestroy */
    if (threadPool->state != RUNNING){
        pthread_mutex_unlock(&(threadPool->lock));
        return;
    }

    /* Assign new state */
    if (shouldWaitForTasks == 0) threadPool->state = HARD_SHUTDOWN;
    else threadPool->state = SOFT_SHUTDOWN;
    pthread_mutex_unlock(&(threadPool->lock));

    /* Wake all pool threads */
    pthread_cond_broadcast(&(threadPool->cnd));

    /* Wait for all threads to finish */
    for(int i = 0; i < threadPool->poolSize; i++) 
    {   
		pthread_join(threadPool->threads[i], NULL);
	}

    /* Dealocate the queue */
    osDestroyQueue(threadPool->queue);
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param)
{
    //printf("Adding task...\n");

	if(threadPool == NULL || computeFunc == NULL) {
        return -1;
    }

    pthread_mutex_lock(&(threadPool->lock));
    /* Check state and create ThreadPoolTask */
    if (threadPool->state != RUNNING) return -1;
    ThreadPoolTask* newTask = malloc(sizeof(ThreadPoolTask));
    if (newTask == NULL) return -1;
    newTask->computeFunc = computeFunc;
    newTask->param = param;

    /* Add task to queue */
    


    osEnqueue(threadPool->queue, newTask);
    threadPool->queueSize++;
    pthread_cond_broadcast(&(threadPool->cnd));

    
    pthread_mutex_unlock(&threadPool->lock);


    return 0;
}

