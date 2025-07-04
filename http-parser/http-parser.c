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
    response->statusCode = -1;
    response->contentLength = -1;
    response->isChunked = 0;
    response->isRedirect = 0;
    response->body = NULL;
}

// create request object from given buffer
int parseHttpRequest(HttpRequest *request, const char *requestBuffer)
{

    if (!requestBuffer || requestBuffer[0] == '\0' || !request)
        return -1;

    // GET /?url=https://google.com/ HTTP/1.1\r\n
    // Host: request->url\r\n
    // User-Agent: MyProxy/1.0\r\n
    // Accept: */*\r\n
    // Connection: close\r\n
    // \r\n

    char *savePtr = NULL;
    char *requestCopy = strdup(requestBuffer);

    if (requestCopy == NULL)
    {
        printf("memory allocation failed for request copy: %s\n", requestCopy);
        return -1;
    }

    if (requestCopy[0] == '\0')
    {
        printf("received empty http request\n");
        free(requestCopy);
        return -1;
    }

    // zero out the request object
    initHttpRequest(request);

    // extract the method reference with \0
    char *method = strtok_r(requestCopy, " ", &savePtr);

    // extract the path reference with \0
    char *path = strtok_r(NULL, " ", &savePtr);

    // extract the http-version reference with \0
    char *httpVersion = strtok_r(NULL, "\r\n", &savePtr);

    // copy the method
    strncpy(request->method, method, strlen(method));

    // copy the http-version
    if (sscanf(httpVersion, "HTTP/%s", request->httpVersion) != 1)
    {
        free(requestCopy);
        printf("failed to extract http-version!\n");
        return -1;
    }

    // now extract host and accept headers
    char hostHeader[HOST_SIZE], acceptHeader[ACCEPT_SIZE];
    char *line = NULL;
    while ((line = strtok_r(NULL, "\r\n", &savePtr)) && line != NULL)
    {
        if (strncmp(line, "Accept:", 7) == 0)
        {
            if (sscanf(line, "Accept: %s", acceptHeader) != 1)
            {
                free(requestCopy);
                printf("failed to extract accept header!\n");
                return -1;
            }
        }
        else if (strncmp(line, "Host:", 5) == 0)
        {
            if (sscanf(line, "Host: %s", hostHeader) != 1)
            {
                free(requestCopy);
                printf("failed to extract host header!\n");
                return -1;
            }
        }
    }

    // put accept as it is
    strncpy(request->accept, acceptHeader, ACCEPT_SIZE);

    // when path not starts with '/' then throw error
    if (strncmp(path, "/", 1) != 0)
    {
        printf("path not starts with '/'\n");
        free(requestCopy);
        return -1;
    }

    // when our home page or favicon.ico is requested then
    // host and path will be same
    if (strcmp(path, "/") == 0 || strcmp(path, "/favicon.ico"))
    {
        // host will remain as it is
        strncpy(request->host, hostHeader, HOST_SIZE);
        strncpy(request->path, path, PATH_SIZE);
    }

    // when query url is requested
    else if (strncmp(path, "/?url=", 6) == 0)
    {
        // skipping the starting /?url=
        path += 6;

        // get the query url
        char *query_url_path = get_url_path_from_query(path);
        strcpy(request->path, query_url_path);
        free(query_url_path);

        // get the host
        char *query_host = get_host_from_query(path);
        strcpy(request->host, query_host);
        free(query_host);

        // creating the filename like url
        int bytes_written = snprintf(request->query_url, URL_SIZE, "%s/%s", request->host, request->path);
        request->query_url[bytes_written] = '\0';
        sanitize_filename(request->query_url);
    }

    printf(
        "\nmethod: %s\n"
        "http-version: %s\n"
        "host: %s\n"
        "path: %s\n"
        "query-url: %s\n"
        "accept: %s\n\n",
        request->method,
        request->httpVersion,
        request->host,
        request->path,
        request->accept);

    free(requestCopy);
    return 1;
}

// create response object from given buffer
int parseHttpResponse(HttpResponse *response, const char *responseBuffer)
{

    if (!response ||
        !responseBuffer ||
        responseBuffer[0] == '\0')
        return -1;

    initHttpResponse(response);

    char *savePtr;
    char *responseCopy = strdup(responseBuffer);

    if (!responseCopy || responseCopy[0] == '\0')
        goto catch;

    // extract the http version
    char *httpVersion = strtok_r(responseCopy, " ", &savePtr);
    if (sscanf(httpVersion, "HTTP/%s", response->httpVersion) != 1)
        goto catch;

    // extract the status code
    char *statusCode = strtok_r(NULL, " ", &savePtr);

    if ((response->statusCode = extractNumber(statusCode, 3)) == -1)
        goto catch;

    // extract the status message
    char *statusMessage = strtok_r(NULL, "\r\n", &savePtr);
    strcpy(response->statusMessage, statusMessage);

    // reading headers
    char *line = NULL;
    while ((line = strtok_r(NULL, "\r\n", &savePtr)) && line != NULL && line[0] != '\0')
    {
        // Content-Type
        // Content-Length
        // Transfer-Encoding: chunked
        // body

        char contentType[sizeof(response->contentType)];

        // extract content-type
        if (strncmp(line, "Content-Type:", 13) == 0)
        {
            if (sscanf(line, "Content-Type: %s", contentType) != 1)
                goto catch;

            char *ptr = contentType;
            while (*ptr == ' ')
                ptr++;

            strcpy(response->contentType, ptr);
        }

        // extract content-length
        if (strncmp(line, "Content-Length:", 15) == 0)
        {
            if (sscanf(line, "Content-Length: %d", &response->contentLength) != 1)
                goto catch;
        }

        // extract the chunked encoding
        if (strncmp(line, "Transfer-Encoding:", 18) == 0)
            response->isChunked = 1;

        // extract location header
        if (strncmp(line, "Location:", 9) == 0)
        {

            char location[URL_SIZE];
            if (sscanf(line, "Location: %s", location) != 1)
                goto catch;

            char *ptr = location;
            while (*ptr == ' ')
                ptr++;

            strcpy(response->location, location);
            response->isRedirect = 1;
        }
    }

    // now its time for body
    char *bodyStart = strstr(responseBuffer, "\r\n\r\n");
    if (bodyStart)
    {
        bodyStart += 4; // skipping "\r\n\r\n"
        if (bodyStart && *bodyStart)
        {
            free(response->body);
            response->body = strdup(bodyStart);
            if (response->body[0] == '\0')
                free(response->body);
        }
    }

    free(responseCopy);

    printf(
        "\nhttp-version: %s\n"
        "status-code: %d\n"
        "status-msg: %s\n"
        "content-type: %s\n"
        "content-length: %d\n"
        "body: %s\n\n",
        response->httpVersion,
        response->statusCode,
        response->statusMessage,
        response->contentType,
        response->contentLength,
        response->body);

    return 1;

catch:
    free(responseCopy);
    return -1;
}

int unparseHttpResponse(HttpResponse *response, char *responseBuffer, size_t bufferSize)
{

    // HTTP/1.1 200 OK\r\n
    // Content-Type : text / html\r\n
    // Location: https://youtube.com
    // Content-Length : 27\r\n
    // <html> Hello World !</ html>

    if (!response || !responseBuffer || bufferSize == 0)
        return -1;

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

    // add starting headers
    int bytesWritten = snprintf(responseBuffer,
                                bufferSize,
                                "HTTP/%s %d %s\r\n"
                                "Content-Type: %s\r\n",
                                response->httpVersion,
                                response->statusCode,
                                response->statusMessage,
                                response->contentType);

    // conditionally add redirect url
    if (response->isRedirect)
    {
        printf("adding redirecting url to response\n");
        bytesWritten += snprintf(responseBuffer + bytesWritten,
                                 bufferSize - bytesWritten,
                                 "Location: %s\r\n",
                                 response->location);
    }

    // add rest headers and body
    bytesWritten += snprintf(responseBuffer + bytesWritten,
                             bufferSize - bytesWritten,
                             "%s: %s\r\n"
                             "\r\n"
                             "%s",
                             contentHeader[0],
                             contentHeader[1],
                             response->body ? response->body : "");

    return bytesWritten;
}

int unparseHttpRequest(HttpRequest *request, char *requestBuffer, size_t bufferSize)
{

    if (!request || !requestBuffer || bufferSize == 0)
        return -1;

    // create a raw request
    int bytesWritten = snprintf(requestBuffer, bufferSize,
                                "%s %s HTTP/%s\r\n"
                                "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\n"
                                "Accept-Encoding: gzip, deflate, br\r\n"
                                "Accept-Language: en-US,en;q=0.9\r\n"
                                "Host: %s\r\n"
                                "Accept: %s\r\n"
                                "Referrer: %s\r\n"
                                "Connection: keep-alive\r\n"
                                "\r\n",
                                request->method,
                                request->path,
                                request->httpVersion,
                                request->host,
                                request->accept,
                                request->host);

    return bytesWritten;
}

// free the dynamically allocated props of response object
void freeHttpResponse(HttpResponse *response)
{
    if (response && response->body)
    {
        free(response->body);
        response->body = NULL;
    }
}