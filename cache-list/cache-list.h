#ifndef CACHE_LIST_H
#define CACHE_LIST_H

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include "../utils/custom-utilities.h"
#define MAX_CACHE_SIZE 100
#define CACHE_DIR "cached"

extern size_t cacheSize;

typedef struct CacheNode
{
    const char *url;
    struct CacheNode *prev;
    struct CacheNode *next;
} CacheNode;

typedef struct ListPair
{
    CacheNode *head;
    CacheNode *tail;
} ListPair;

// checks if the cache is empty
int isCacheEmpty(const CacheNode *tail);

// checks if the cache is full
int isCacheFull(const CacheNode *head);

// initializes cache node with default values
void initCacheNode(CacheNode *node);

// add node to cache head
CacheNode *addCacheNode(CacheNode *head, const char *url, const int url_size, const char *data, const char *data_type);

// remove node from cache tail
CacheNode *removeCacheNode(CacheNode *tail);

// search node in cache, NULL for not found
CacheNode *findCacheNode(const CacheNode *head, const char *url);

// move a node to head
CacheNode *moveToHead(CacheNode *head, CacheNode *node);

// populate cache with the disk cached items, returns head
ListPair populateCacheWithDisk();

#endif