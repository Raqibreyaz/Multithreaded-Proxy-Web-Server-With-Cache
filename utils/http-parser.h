#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define _POSIX_C_SOURCE 200809
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HEADERS 20

typedef struct
{
    char *key;
    char *value;
} HttpHeader;

typedef struct
{
    int statusCode;          // Response Status Code (200,404 etc.)
    char statusMessage[128]; // Status Message (OK, Internal Server Error)
    char httpVersion[16];    // The HTTP version received from remote server
    HttpHeader *headers;     // Response Headers
    int headerCount;         // Number of Headers
    char *body;              // Response Body
} HttpResponse;

typedef struct
{
    char method[8];       // GET, POST, etc.
    char url[256];        // Requested URL
    char httpVersion[16]; // HTTP/1.1, HTTP/2, etc
    HttpHeader *headers;  // Requested Headers
    int headerCount;      // Number of Headers
    char *body;           // Request body (dynamic allocation)
} HttpRequest;

void initHttpRequest(HttpRequest *request);

void initHttpResponse(HttpResponse *response);

int parseHttpRequest(const char *rawRequest, HttpRequest *request);

int parseHttpResponse(const char *responseBuffer, HttpResponse *response);

char *unparseHttpResponse(HttpResponse *response);

void freeHttpRequest(HttpRequest *request);

void freeHttpResponse(HttpResponse *response);

#endif