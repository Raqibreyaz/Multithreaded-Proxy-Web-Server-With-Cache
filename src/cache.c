#include "../include/cache.h"

CacheLRU *init_cache_lru(int max_size)
{
    CacheLRU *cache = (CacheLRU *)malloc(sizeof(CacheLRU));
    cache->head = NULL;
    cache->tail = NULL;
    cache->current_size = 0;
    cache->max_size = max_size;

    // open the directory for taking cache filenames
    DIR *dir = opendir(CACHE_DIR);

    // traversing through entries in the directory and adding their names in cache
    struct dirent *de;
    struct stat st;
    char full_path[1024];
    while ((de = readdir(dir)) != NULL)
    {
        // skip unwanted entries like '.', '..', '.meta'
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        // taking the full path
        snprintf(full_path, sizeof(full_path), "%s/%s", CACHE_DIR, de->d_name);

        // taking the stats of the file
        if (stat(full_path, &st) == -1)
        {
            perror("stat failed");
            closedir(dir);
            free_cache_lru(cache);
            return NULL;
        }

        // if it is a regular file then add name in cache
        if (S_ISREG(st.st_mode))
            lru_insert(cache, de->d_name, NULL, 0, NULL);
    }

    // closing the directory
    closedir(dir);

    // returning the head of final cache
    return cache;
}

void free_cache_lru(CacheLRU *cache)
{
    if (!cache)
        return;

    CacheEntry *curr = cache->head;
    while (curr)
    {
        CacheEntry *next = curr->next;
        free(curr->url);
        free(curr);
        curr = next;
    }
    free(cache);
}

int lru_contains(CacheLRU *cache, const char *url)
{

    if (!cache || !url || url[0] == '\0')
        return 0;

    CacheEntry *tmp = cache->head;
    char *filename = get_cache_filename(url);

    char full_path[1024] = {0};
    snprintf(full_path, sizeof(full_path) - 1, "%s/%s", CACHE_DIR, filename);

    // finding the node which has that url
    while (tmp &&
           tmp->url &&
           (!urls_are_equivalent(tmp->url, filename)))
        tmp = tmp->next;

    free(filename);

    // if >2 hours are passed of the cache then cache shouldn't exist
    if (tmp && time_passed_in_hours_for_file(full_path) > 2)
    {
        printf("cache invalidated for: %s\n", url);
        return 0;
    }

    // return the node  if found or null
    return tmp != NULL;
}

void lru_touch(CacheLRU *cache, const char *url)
{
    CacheEntry *curr = cache->head;
    char *filename = get_cache_filename(url);
    while (curr)
    {
        if (strcmp(curr->url, filename) == 0)
        {
            // Already at head
            if (curr == cache->head)
                return;

            // Detach from current position
            if (curr->prev)
                curr->prev->next = curr->next;
            if (curr->next)
                curr->next->prev = curr->prev;

            if (curr == cache->tail)
                cache->tail = curr->prev;

            // Move to head
            curr->prev = NULL;
            curr->next = cache->head;
            if (cache->head)
                cache->head->prev = curr;
            cache->head = curr;
            free(filename);
            return;
        }
        curr = curr->next;
    }
    free(filename);
}

void lru_insert(CacheLRU *cache, const char *url, const char *data, size_t data_len, const char *content_type)
{
    if (!cache || !url)
        return;

    char *filename = get_cache_filename(url);

    // if url already in the list then skip
    if (lru_contains(cache, filename))
    {
        lru_touch(cache, filename);
        free(filename);
        return;
    }

    // remove least recently used cache
    if (cache->current_size >= cache->max_size)
        lru_evict(cache);

    // Step 1: Write to disk
    if (data && content_type)
        write_cache_file(filename, content_type, data, data_len);

    // Step 2: Create new LRU entry
    CacheEntry *entry = malloc(sizeof(CacheEntry));
    if (!entry)
    {
        printf("failed to add entry in list\n");
        free(filename);
        return;
    }

    entry->url = strdup(filename);
    entry->prev = NULL;
    entry->next = cache->head;

    if (cache->head)
        cache->head->prev = entry;
    cache->head = entry;

    if (!cache->tail)
        cache->tail = entry;

    cache->current_size++;
    free(filename);
}

void lru_evict(CacheLRU *cache)
{
    if (!cache || !cache->tail)
        return;

    CacheEntry *victim = cache->tail;

    if (victim->prev)
        victim->prev->next = NULL;
    else
        cache->head = NULL;

    cache->tail = victim->prev;

    if (victim->url)
        remove(victim->url);

    free(victim->url);
    free(victim);

    cache->current_size--;
}

void lru_delete(CacheLRU *cache, const char *url)
{
    CacheEntry *curr = cache->head;

    char *filename = get_cache_filename(url);

    while (curr)
    {
        if (strcmp(curr->url, filename) == 0)
        {
            if (curr->prev)
                curr->prev->next = curr->next;
            else
                cache->head = curr->next;

            if (curr->next)
                curr->next->prev = curr->prev;
            else
                cache->tail = curr->prev;

            // remove the corresponding file
            if (curr->url)
                remove(curr->url);

            free(filename);
            free(curr->url);
            free(curr);
            cache->current_size--;
            return;
        }
        curr = curr->next;
    }

    free(filename);
}

// print the cache list urls
void print_cache_list(CacheLRU *cache)
{
    if (!cache || cache->current_size == 0)
        return;

    printf("\ncurrent_list_scenario:\n");
    CacheEntry *temp = cache->head;
    while (temp)
    {
        printf("%s\n", temp->url);
        temp = temp->next;
    }
    printf("\n");
}