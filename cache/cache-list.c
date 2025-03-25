#include "cache-list.h"

// will check if cache is full
int isCacheFull(CacheNode *head)
{
    return head != NULL && cacheSize >= MAX_CACHE_SIZE;
}

// will check if cache is empty
int isCacheEmpty(CacheNode *head)
{
    return cacheSize < 1;
}

// will initialize the cache node
void initCacheNode(CacheNode *node)
{
    node->data = NULL;
    node->next = NULL;
    node->prev = NULL;
}

// will add the cache data to list at front
CacheNode *addCacheNode(CacheNode *head, HttpResponse *data)
{
    if (head == NULL || data == NULL || isCacheFull(head))
        return NULL;

    // allocate space and initialize the node
    CacheNode *node = (CacheNode *)malloc(sizeof(CacheNode));
    initCacheNode(node);

    node->data = data;
    node->next = head;

    // increase size of cache
    cacheSize++;

    return node;
}

// will remove least recently used cache from back
CacheNode *removeCacheNode(CacheNode *tail)
{
    if (tail != NULL)
    {

        CacheNode *temp = tail;
        tail = tail->prev;

        // tail can be null when there was only one node
        if (tail != NULL)
            tail->next = NULL;

        // free that node
        free(temp);

        // decrease size of cache
        cacheSize--;
    }
    return tail;
}

HttpResponse *findCacheNode(CacheNode *head, const char *host, const char *path)
{
    while (
        head != NULL &&
        (strcmp(head->data->host, host) != 0 || strcmp(head->data->path, path)) != 0)
        head = head->next;

    // return the http response if found
    return head == NULL ? NULL : head->data;
}