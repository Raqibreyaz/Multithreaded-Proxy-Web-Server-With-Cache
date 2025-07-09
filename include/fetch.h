#ifndef FETCH_H
#define FETCH_H
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <openssl/ssl.h>
#include "cache.h"
#include "utils.h"
#include "http-parser.h"
#include "socket-utils.h"
#include "html-rewriter.h"
#include "http-request-response.h"

#define URL_MAX_LEN 2048

typedef struct ParsedURL
{
    char scheme[8];  // "http" or "https"
    char host[256];  // e.g. "example.com"
    char port[6];    // e.g. "80" or "443"
    char path[1024]; // e.g. "/index.html"
} ParsedURL;

// Parses a URL into components (http/https, host, port, path)
int parse_url(const char *url, ParsedURL *out);

// Main fetch function: performs HTTP/HTTPS GET request
// - `url` is the target URL
// - `max_redirects` defines how many redirects it should follow
// Returns a heap-allocated HttpResponse*, or NULL on error
struct HttpResponse *fetch_url(const char *url, int max_redirects);

struct HttpResponse *fetch_cache_or_url(CacheLRU *cache, const char *url, int max_redirects);

#endif