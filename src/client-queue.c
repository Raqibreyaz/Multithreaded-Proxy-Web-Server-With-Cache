#include "../include/client-queue.h"

void init_client_queue(ClientQueue *q)
{
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

// add client to queue by locking the queue
void enqueue_client(ClientQueue *q, int client_sock)
{
    ClientQueueNode *node = malloc(sizeof(ClientQueueNode));
    node->client_sock = client_sock;
    node->next = NULL;

    pthread_mutex_lock(&q->lock);
    if (q->rear)
    {
        q->rear->next = node;
        q->rear = node;
    }
    else
    {
        q->front = q->rear = node;
    }
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

// remove client from queue, No LOCKS used
int dequeue_client(ClientQueue *q)
{
    if (is_queue_empty(q))
        return -1;

    ClientQueueNode *node = q->front;
    int client_sock = node->client_sock;

    q->front = node->next;
    if (q->front == NULL)
        q->rear = NULL;

    free(node);
    return client_sock;
}

// check whether given queue is empty or not
int is_queue_empty(ClientQueue *client_queue)
{
    return client_queue->front == NULL;
}