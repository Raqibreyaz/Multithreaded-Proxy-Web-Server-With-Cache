#include "../include/thread-pool.h"

void *worker_thread_func(void *arg)
{
    SharedContext *shared_ctx = (SharedContext *)arg;

    if (!shared_ctx)
        return NULL;

    while (1)
    {
        pthread_mutex_lock(&(shared_ctx->client_queue->lock));

        // wait until a client comes
        while (is_queue_empty(shared_ctx->client_queue))
        {
            pthread_cond_wait(&(shared_ctx->client_queue->not_empty), &(shared_ctx->client_queue->lock));
        }

        // remove the client from queue
        int client_sock = dequeue_client(shared_ctx->client_queue);

        // when no client then skip
        if (client_sock < 0)
        {
            pthread_mutex_unlock(&(shared_ctx->client_queue->lock));
            continue;
        }

        ClientHandlerArgs *args = (ClientHandlerArgs *)calloc(1, sizeof(ClientHandlerArgs));

        // re_add to queue if memory allocation failed
        if (!args)
        {
            enqueue_client(shared_ctx->client_queue, client_sock);
            pthread_mutex_unlock(&(shared_ctx->client_queue->lock));
            continue;
        }

        pthread_mutex_unlock(&(shared_ctx->client_queue->lock));

        // initialize client args
        args->client_fd = client_sock;
        args->blocked_sites = shared_ctx->blocked_sites;
        args->cache = shared_ctx->cache;
        args->cache_lock = shared_ctx->cache_lock;
        args->n_of_b_sites = shared_ctx->n_of_b_sites;

        handle_client((void *)args);

        free(args);
    }

    return NULL;
}

void init_thread_pool(SharedContext *shared_ctx)
{
    pthread_t threads[THREAD_POOL_SIZE];

    // create the exact no of threads with passing shared context to each
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        if (pthread_create(&threads[i], NULL, worker_thread_func, shared_ctx) != 0)
        {
            perror("pthread_create");
        }
        pthread_detach(threads[i]);
    }
}
