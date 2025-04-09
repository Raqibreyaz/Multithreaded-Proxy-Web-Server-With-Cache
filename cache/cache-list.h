#ifndef CACHE_LIST_H
#define CACHE_LIST_H

#include "../http-parser/http-parser.h"

#define MAX_CACHE_SIZE 20

extern size_t cacheSize;

typedef struct CacheNode
{
    HttpResponse *data;
    struct CacheNode *prev;
    struct CacheNode *next;
} CacheNode;

int isCacheEmpty();
int isCacheFull(CacheNode *head);
void initCacheNode(CacheNode *node);
CacheNode *addCacheNode(CacheNode *head, HttpResponse *data);
CacheNode *removeCacheNode(CacheNode *tail);
CacheNode *findCacheNode(CacheNode *head, const char *host, const char *path);
CacheNode *moveToHead(CacheNode *head, CacheNode *node);

#endif