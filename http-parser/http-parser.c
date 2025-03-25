#include "http-parser.h"

// for request
// char method[];
// char host[];
// char path[];
// char httpVersion[];
// char accept[];

// for response
// char httpVersion[]
// int statusCode;
// char statusMessage[];
// int contentLength;
// int isChunked;
// char *body;

// zero out the request object
void initHttpRequest(HttpRequest *request)
{
    memset(request, 0, sizeof(HttpRequest));
}

// zero out the response object
void initHttpResponse(HttpResponse *response)
{
    memset(response, 0, sizeof(HttpResponse));
    response->body = NULL;
}

// create request object from given buffer
int parseHttpRequest(const char *requestBuffer, HttpRequest *request)
{

    if (!requestBuffer || !request)
        return -1;

    // GET / HTTP/request->httpVersion\r\n
    // Host: request->url\r\n
    // User-Agent: MyProxy/1.0\r\n
    // Accept: */*\r\n
    // Connection: close\r\n
    // \r\n

    char *savePtr = NULL;
    char *requestCopy = strdup(requestBuffer);

    // zero out the request object
    initHttpRequest(request);

    // extract the method reference with \0
    char *method = strtok_r(requestCopy, " ", &savePtr);

    // when the request is not GET then throw error
    if (strcmp(method, "GET") != 0)
    {
        free(requestCopy);
        return -1;
    }

    // extract the path reference with \0
    char *path = strtok_r(NULL, " ", &savePtr);
    // extract the http-version reference with \0
    char *httpVersion = strtok_r(NULL, "\r\n", &savePtr);

    // copy the method
    strncpy(request->method, method, strlen(method));
    // copy the http-version
    sscanf(httpVersion, "HTTP/%s", request->httpVersion);

    // now extract host and accept headers
    char hostHeader[HOST_SIZE],
        acceptHeader[ACCEPT_SIZE];
    char *line = NULL;
    while ((line = strtok_r(NULL, "\r\n", &savePtr)) && strlen(line) > 0)
    {
        if (strncmp(line, "Accept:", 7) == 0)
        {
            sscanf(line, "Accept: %s", acceptHeader);
        }
        else if (strncmp(line, "Host:", 5) == 0)
        {
            sscanf(line, "Host: %s", hostHeader);
        }
    }

    // put accept as it is
    strncpy(request->accept, acceptHeader, ACCEPT_SIZE);

    // when our home page is requested
    if (strcmp(path, "/") == 0)
    {
        // host will remain as it is
        strncpy(request->host, hostHeader, HOST_SIZE);
        strncpy(request->path, path, PATH_SIZE);
    }
    // when a path is given after /
    else if (strncmp(path, "/", 1) == 0 && strcmp(path, "/favicon.ico") != 0)
    {
        // point to the slash part for extracting path
        char *slash = strchr(path + 1, '/');
        char *host = path + 1;

        int hostLen = 0;
        int pathLen = 0;

        // when there is no path then assign /
        if (!slash)
        {
            // assign / to path
            strcpy(request->path, "/");
            // assign the host directly
            strncpy(request->host, host, sizeof(request->host));
            // assign the host length directly
            hostLen = strlen(host);
        }
        // when there is a path exist
        else
        {
            // extract the path length
            pathLen = strlen(slash);
            // extract the host length
            hostLen = slash - host;

            // copy the required length of host
            strncpy(request->host, host, hostLen);
            // copy the required length of path
            strncpy(request->path, slash, pathLen);
        }
    }
    // when invalid path is given then throw error
    else
    {
        free(requestCopy);
        return 0;
    }

    free(requestCopy);

    return 1;
}

// create response object from given buffer
int parseHttpResponse(const char *responseBuffer, HttpResponse *response)
{
    // HTTP/1.1 200 OK\r\n
    // Content - Type : text / html\r\n
    // Content - Length : 27\r\n
    // Server : Apache\r\n\r\n
    // <html> Hello World !</ html>

    return 0;
}

int unparseHttpResponse(HttpResponse *response, char *responseBuffer, size_t bufferSize)
{

    // HTTP/1.1 200 OK\r\n
    // Content-Type : text / html\r\n
    // Content-Length : 27\r\n
    // Server : Apache\r\n\r\n
    // <html> Hello World !</ html>

    char contentHeader[2][30];

    if (response->isChunked)
    {
        strcpy(contentHeader[0], "Transfer-Encoding");
        strcpy(contentHeader[1], "chunked");
    }
    else
    {
        strcpy(contentHeader[0], "Content-Length");
        sprintf(contentHeader[1], "%d", response->contentLength);
    }

    int bytesWritten = snprintf(responseBuffer, bufferSize,
                                "HTTP/%s %d %s\r\n"
                                "Content-Type: %s\r\n"
                                "%s: %s\r\n"
                                "Connection: close"
                                "\r\n\r\n"
                                "%s",
                                response->httpVersion,
                                response->statusCode,
                                response->statusMessage,
                                response->contentType,
                                contentHeader[0],
                                contentHeader[1],
                                response->body);

    return bytesWritten;
}

int unparseHttpRequest(HttpRequest *request, char *requestBuffer, size_t bufferSize)
{

    // create a raw request
    int bytesWritten = snprintf(requestBuffer, bufferSize,
                                "%s %s HTTP/%s\r\n"
                                "Host: %s\r\n"
                                "Accept: %s\r\n"
                                "Connection: close\r\n"
                                "\r\n",
                                request->method,
                                request->path,
                                request->httpVersion,
                                request->host,
                                request->accept);

    return bytesWritten;
}

// free the dynamically allocated props of response object
void freeHttpResponse(HttpResponse *response)
{
    free(response->body);
}