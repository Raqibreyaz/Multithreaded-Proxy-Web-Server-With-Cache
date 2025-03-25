#ifndef CACHE_LIST_H
#define CACHE_LIST_H

#include "../http-parser/http-parser.h"

#define MAX_CACHE_SIZE 20

int cacheSize = 0;

typedef struct CacheNode
{
    HttpResponse *data;
    struct CacheNode *prev;
    struct CacheNode *next;
} CacheNode;

int isCacheFull(CacheNode *head);
int isCacheEmpty(CacheNode *head);
void initCacheNode(CacheNode *node);
CacheNode *addCacheNode(CacheNode *head, HttpResponse *data);
CacheNode *removeCacheNode(CacheNode *tail);
HttpResponse *findCacheNode(CacheNode *head, const char *host, const char *path);

#endif