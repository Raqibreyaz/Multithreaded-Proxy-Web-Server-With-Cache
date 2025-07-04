#include "socket-library/socket-library.h"
#include "cache/cache-list.h"
#include <pthread.h>

#define BACKLOG_SIZE 10
#define MAX_CLIENTS 10
#define PORT 3000

// forward request to remote server
// received response from server and send back to client
int main(int argc, char const *argv[])
{
    int liveThreads = 0;

    // creating static mutex and cond vars
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    struct sockaddr_in myAddr;
    CacheNode *head = NULL;
    CacheNode *tail = NULL;

    // create a server at loopback address
    int myFd = createServer(AF_INET, SOCK_STREAM, PORT, BACKLOG_SIZE, "127.0.0.1", (struct sockaddr_storage *)&myAddr);

    while (1)
    {
        int cfd;

        // wait for clients to finish to get free space
        pthread_mutex_lock(&mtx);
        while (liveThreads >= MAX_CLIENTS)
        {
            printf("main thread is waiting for signal\n");
            pthread_cond_wait(&cond, &mtx);
        }
        pthread_mutex_unlock(&mtx);

        // accept client connection
        if ((cfd = acceptClient(myFd, NULL, NULL)) == -1)
        {
            printf("failed to connect to client\n");
            continue;
        }

        printf("client connected\n");

        struct ClientHandlerArg *arg =
            (struct ClientHandlerArg *)malloc(sizeof(struct ClientHandlerArg));

        arg->cfd = cfd;
        arg->head = &head;
        arg->tail = &tail;
        arg->cond = &cond;
        arg->mtx = &mtx;
        arg->liveThreads = &liveThreads;

        pthread_t thread;

        // new thread is created
        liveThreads++;

        if (pthread_create(&thread, NULL, handleClient, arg) != 0)
        {
            perror("pthread_create");
            close(cfd);
            free(arg);
            liveThreads--;
            continue;
        }

        printf("no of running threads: %d\n", liveThreads);

        pthread_detach(thread);
    }
    return 0;
}
