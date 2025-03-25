#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define METHOD_SIZE 8
#define HOST_SIZE 256
#define PATH_SIZE 512
#define HTTP_VERSION_SIZE 16
#define ACCEPT_SIZE 512
#define CONTENT_TYPE_SIZE 100
#define STATUS_MESSAGE_SIZE 64

typedef struct
{
    char method[METHOD_SIZE];
    char host[HOST_SIZE];
    char path[PATH_SIZE];
    char httpVersion[HTTP_VERSION_SIZE];
    char accept[ACCEPT_SIZE];
} HttpRequest;

typedef struct
{
    int statusCode;
    char httpVersion[HTTP_VERSION_SIZE];
    char statusMessage[STATUS_MESSAGE_SIZE];
    char contentType[CONTENT_TYPE_SIZE];
    char host[HOST_SIZE];
    char path[PATH_SIZE];
    int contentLength;
    int isChunked;
    char *body;
} HttpResponse;

void initHttpRequest(HttpRequest *request);

void initHttpResponse(HttpResponse *response);

int parseHttpRequest(const char *rawRequest, HttpRequest *request);

int parseHttpResponse(const char *responseBuffer, HttpResponse *response);

int unparseHttpResponse(HttpResponse *response, char *responseBuffer, size_t bufferSize);

int unparseHttpRequest(HttpRequest *request, char *requestBuffer, size_t bufferSize);

void freeHttpResponse(HttpResponse *response);

#endif