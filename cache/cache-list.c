#include "cache-list.h"

size_t cacheSize = 0;

// will check if cache is full
int isCacheFull(CacheNode *head)
{
    return head != NULL && cacheSize >= MAX_CACHE_SIZE;
}

// will check if cache is empty
int isCacheEmpty()
{
    return cacheSize < 1;
}

// will initialize the cache node
void initCacheNode(CacheNode *node)
{
    if (!node)
        return;

    node->data = NULL;
    node->next = NULL;
    node->prev = NULL;
}

// will add the cache data to list at front
CacheNode *addCacheNode(CacheNode *head, HttpResponse *data)
{
    if (data == NULL || isCacheFull(head))
        return NULL;

    // allocate space and initialize the node
    CacheNode *node = (CacheNode *)malloc(sizeof(CacheNode));
    if (!node)
    {
        printf("memory allocation failed for cache node\n");
        return NULL;
    }

    HttpResponse *res = (HttpResponse *)malloc(sizeof(HttpResponse));
    if (!res)
    {
        printf("memory allocation failed for data in cache node\n");
        free(node);
        return NULL;
    }

    initCacheNode(node);
    initHttpResponse(res);

    // copy the values of data to res
    memcpy(res, data, sizeof(HttpResponse));
    res->body = data->body ? strdup(data->body) : NULL;

    node->data = res;
    node->next = head;
    if (head)
        head->prev = node;

    // increase size of cache
    cacheSize++;

    printf("data added to cache\n");

    return node;
}

// will remove least recently used cache from back
CacheNode *removeCacheNode(CacheNode *tail)
{
    if (tail)
    {
        CacheNode *temp = tail;
        tail = tail->prev;

        if (tail)
            tail->next = NULL;

        if (temp->data)
        {
            free(temp->data->body); // Safe even if NULL
            free(temp->data);
        }

        free(temp);
        cacheSize--;

        printf("data removed from cache\n");
    }
    return tail;
}

CacheNode *findCacheNode(CacheNode *head, const char *host, const char *path)
{

    if (!host || !path || host[0] == '\0' || path[0] == '\0')
        return NULL;

    while (head &&
           head->data &&
           head->data->host[0] != '\0' &&
           head->data->path[0] != '\0' &&
           (strcmp(head->data->host, host) != 0 || strcmp(head->data->path, path) != 0))
    {
        head = head->next;
    }

    // return the http response if found
    return head;
}

// will return the new tail if it is updated
CacheNode *moveToHead(CacheNode *head, CacheNode *node)
{
    if (!head || !node || head == node)
        return NULL;

    CacheNode *prev = node->prev;
    CacheNode *next = node->next;

    node->prev = NULL;
    node->next = head;
    head->prev = node;

    if (next)
    {
        next->prev = prev;
    }
    else
    {
        printf("tail node modified\n");
    }

    if (prev)
        prev->next = next;

    printf("cache moved to head\n");

    return next == NULL ? prev : NULL;
}