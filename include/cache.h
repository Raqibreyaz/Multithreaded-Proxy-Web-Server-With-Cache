#ifndef CACHE_H
#define CACHE_H
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "utils.h"
#include "cache-store.h"

typedef struct CacheEntry
{
    char *url; // original URL
    struct CacheEntry *prev;
    struct CacheEntry *next;
} CacheEntry;

typedef struct
{
    CacheEntry *head;
    CacheEntry *tail;
    int max_size;
    int current_size;
} CacheLRU;

CacheLRU *init_cache_lru(int max_size);

void free_cache_lru(CacheLRU *cache);

int lru_contains(CacheLRU *cache, const char *url);

void lru_touch(CacheLRU *cache, const char *url);

void lru_insert(CacheLRU *cache, const char *url, const char *data, size_t data_len, const char *content_type);

void lru_evict(CacheLRU *cache);

void lru_delete(CacheLRU *cache, const char *url);

void print_cache_list(CacheLRU *cache);
#endif