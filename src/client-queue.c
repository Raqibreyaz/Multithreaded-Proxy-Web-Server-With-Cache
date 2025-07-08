#include "../include/client-queue.h"

void init_client_queue(ClientQueue *q)
{
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

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

int dequeue_client(ClientQueue *q)
{
    pthread_mutex_lock(&q->lock);

    // wait if queue is empty
    while (is_queue_empty(q))
    {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    ClientQueueNode *node = q->front;
    int client_sock = node->client_sock;

    q->front = node->next;
    if (q->front == NULL)
        q->rear = NULL;

    free(node);
    pthread_mutex_unlock(&q->lock);
    return client_sock;
}

int is_queue_empty(ClientQueue *client_queue)
{
    return client_queue->front == NULL;
}