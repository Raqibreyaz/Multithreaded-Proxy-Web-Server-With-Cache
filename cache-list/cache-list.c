#include "cache-list.h"

size_t cacheSize = 0;

// will check if cache is full
int isCacheFull(const CacheNode *head)
{
    return head != NULL && cacheSize >= MAX_CACHE_SIZE;
}

// will check if cache is empty
int isCacheEmpty(const CacheNode *tail)
{
    return tail == NULL && cacheSize < 1;
}

// will initialize the cache node
void initCacheNode(CacheNode *node)
{
    if (!node)
        return;

    node->url = NULL;
    node->next = NULL;
    node->prev = NULL;
}

// will add the url to list at head
CacheNode *addCacheNode(CacheNode *head, const char *url, const int url_size, const char *data, size_t data_size, const char *data_type, size_t data_type_size)
{
    if (url == NULL || isCacheFull(head))
        return NULL;

    if ((data != NULL && data_type == NULL) || (data == NULL && data_type != NULL))
    {
        printf("data and its type both are required for writing to file!!\n");
        return NULL;
    }

    // allocate space and initialize the node
    CacheNode *node = (CacheNode *)malloc(sizeof(CacheNode));
    if (!node)
    {
        printf("memory allocation failed for cache node\n");
        return NULL;
    }
    initCacheNode(node);

    // allocate space for the url
    node->url = (char *)malloc(url_size);
    if (!node->url)
    {
        printf("memory allocation failed for url in cache node\n");
        free(node);
        return NULL;
    }

    // copy the values of url to node->url
    node->url = strdup(url);

    // adding the new node to head
    node->next = head;
    if (head)
        head->prev = node;

    // write the data to file in disk
    if (data && data_type)
    {
        char file_path[1024];

        // writing data
        snprintf(file_path, sizeof(file_path), "%s/%s", CACHE_DIR, url);
        write_file(file_path, data, data_size);

        // writing the type of data
        snprintf(file_path, sizeof(file_path), "%s/%s.meta", CACHE_DIR, url);
        write_file(file_path, data_type, data_type_size);
    }

    // increase size of cache
    cacheSize++;

    printf("url added to cache\n");

    // return the new head node
    return node;
}

// returns updated tail node by removing least recently used cache node from back
CacheNode *removeCacheNode(CacheNode *tail)
{
    // avoid if cache is empty
    if (!isCacheEmpty(tail))
    {
        // delete the file from disk
        char file_path[1024];
        snprintf(file_path, sizeof(file_path), "%s/%s", CACHE_DIR, tail->url);
        delete_file(file_path);

        // delete its data type from disk
        snprintf(file_path, sizeof(file_path), "%s/%s.meta", CACHE_DIR, tail->url);
        delete_file(file_path);

        CacheNode *temp = tail;

        // move tail to prev node
        tail = tail->prev;

        // remove link to last node to null
        if (tail)
            tail->next = NULL;

        // free the allocated url memory of removed node
        if (temp->url)
            free(temp->url); // Safe even if NULL

        // now free the removed node
        free(temp);

        // decrease the cache size
        cacheSize--;

        printf("url removed from cache\n");
    }

    // return the updated tail node
    return tail;
}

// returns the node having that url, NULL if not found
CacheNode *findCacheNode(CacheNode *head, const char *url)
{

    if (!head || !url || url[0] == '\0')
        return NULL;

    // finding the node which has that url
    while (head &&
           head->url &&
           (strcmp(head->url, url) != 0))
        head = head->next;

    // return the node  if found or null
    return head;
}

// will return the new tail if it is updated, NULL otherwise
CacheNode *moveToHead(CacheNode *head, CacheNode *node)
{
    if (!head || !node || head == node)
        return NULL;

    // divide the list into 2 parts from 'node'
    CacheNode *prev = node->prev;
    CacheNode *next = node->next;

    // moving 'node' to head
    node->prev = NULL;
    node->next = head;
    head->prev = node;

    // if 2nd part is not NULL then link it to 1st part
    if (next)
        next->prev = prev;
    else
        printf("tail node modified\n");

    // if 1st part is not null then link it to 2nd part
    if (prev)
        prev->next = next;

    printf("cache moved to head\n");

    // returning new tail node if tail modified, NULL otherwise
    return next == NULL ? prev : NULL;
}

// returns the head and tail of the created list
ListPair populateCacheWithDisk()
{
    ListPair list_pair = {.head = NULL, .tail = NULL};

    // open the directory for taking cache filenames
    DIR *dir = opendir(CACHE_DIR);

    // traversing through entries in the directory and adding their names in cache
    struct dirent *de;
    struct stat st;
    char full_path[1024];
    while ((de = readdir(dir)) != NULL)
    {
        // skip unwanted entries like '.', '..', '.meta'
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || strstr(de->d_name, ".meta"))
            continue;

        // taking the full path
        snprintf(full_path, sizeof(full_path), "%s/%s", CACHE_DIR, de->d_name);

        // taking the stats of the file
        if (stat(full_path, &st) == -1)
        {
            perror("stat failed");
            ListPair lp = {.head = NULL, .tail = NULL};
            return lp;
        }

        // if it is a regular file then add name in cache
        if (S_ISREG(st.st_mode))
        {
            // for tracking 1st node
            CacheNode *tmp = list_pair.head;

            // create and add node to the list
            list_pair.head = addCacheNode(list_pair.head, de->d_name, strlen(de->d_name), NULL, 0, NULL, 0);

            // if this is 1st node then point tail to it
            if (!tmp)
                list_pair.tail = list_pair.head;
        }
    }

    // closing the directory
    closedir(dir);

    // returning the head of final cache
    return list_pair;
}