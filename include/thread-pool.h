#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "client-queue.h"
#include "client-handler.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define THREAD_POOL_SIZE 10

typedef struct
{
    ClientQueue *client_queue;
    CacheLRU *cache;
    pthread_mutex_t *cache_lock;
    char **blocked_sites;
    int n_of_b_sites;
} SharedContext;

void init_thread_pool(SharedContext *shared_ctx);
void *worker_thread_func(void *arg);

#endif
