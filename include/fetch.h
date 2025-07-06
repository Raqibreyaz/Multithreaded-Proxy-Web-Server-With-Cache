#ifndef FETCH_H
#define FETCH_H

#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h>

#define INITIAL_BUFFER_SIZE 8192

typedef struct
{
    char scheme[8];  // "http" or "https"
    char host[256];  // e.g. "example.com"
    char port[6];    // e.g. "80" or "443"
    char path[1024]; // e.g. "/index.html"
} ParsedURL;

typedef struct
{
    int statusCode;
    char httpVersion[16];   // "HTTP/1.1"
    char statusMessage[64]; // "OK"
    char contentType[64];   // "text/html; charset=UTF-8"
    int contentLength;      // -1 if not present
    int isChunked;          // 1 if Transfer-Encoding: chunked
    char *body;             // heap-allocated
    size_t bodyLength;      // actual number of bytes in body
    int isRedirect;
    char location[512];
} HttpResponse;

// Parses a URL into components (http/https, host, port, path)
int parse_url(const char *url, ParsedURL *out);

// Frees the memory allocated by fetch_url
void free_http_response(HttpResponse *res);

// Main fetch function: performs HTTP/HTTPS GET request
// - `url` is the target URL
// - `max_redirects` defines how many redirects it should follow
// Returns a heap-allocated HttpResponse*, or NULL on error
HttpResponse *fetch_url(const char *url, int max_redirects);

#endif