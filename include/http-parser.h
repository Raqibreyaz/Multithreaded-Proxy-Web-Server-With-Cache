#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include "fetch.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct HttpResponse
{
    int statusCode;
    char httpVersion[16];   // "HTTP/1.1"
    char statusMessage[64]; // "OK"
    char contentType[128];  // "text/html; charset=UTF-8"
    int contentLength;      // -1 if not present
    int isChunked;          // 1 if Transfer-Encoding: chunked
    char *body;             // heap-allocated
    size_t bodyLength;      // actual number of bytes in body
    int isRedirect;         // boolean for redirection checking
    char location[512];     // redirection location
} HttpResponse;

typedef struct HttpRequest
{
    char method[8];
    char path[512];
    char *query;
    char http_version[16];
} HttpRequest;

// parses the raw http response into the http response object
HttpResponse *parse_http_response(const char *raw, size_t raw_len);

HttpRequest* parse_http_request(const char* raw,size_t raw_len);

void init_http_response(HttpResponse *res);

void init_http_request(HttpRequest *res);

// Frees the memory allocated by fetch_url
void free_http_response(HttpResponse *res);

void free_http_request(HttpRequest *req);

#endif