#ifndef CLIENT_QUEUE_H
#define CLIENT_QUEUE_H

#include <pthread.h>

typedef struct ClientQueueNode {
    int client_sock;
    struct ClientQueueNode *next;
} ClientQueueNode;

typedef struct {
    ClientQueueNode *front;
    ClientQueueNode *rear;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} ClientQueue;

void init_client_queue(ClientQueue *q);
void enqueue_client(ClientQueue *q, int client_sock);
int dequeue_client(ClientQueue *q);
int is_queue_empty(ClientQueue *q);

#endif
