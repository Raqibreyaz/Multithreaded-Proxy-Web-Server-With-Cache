#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H
#define MAX_REDIRECTS_ALLOWED 4

#include "http-request-response.h"
#include "http-parser.h"
#include "blocked-sites.h"
#include "cache.h"
#include "utils.h"
#include <unistd.h>

typedef struct
{
    int client_fd;
    CacheLRU *cache;
    char **blocked_sites;
    int n_of_b_sites;
    pthread_mutex_t* cache_lock;
} ClientHandlerArgs;

void* handle_client(void *args);

#endif