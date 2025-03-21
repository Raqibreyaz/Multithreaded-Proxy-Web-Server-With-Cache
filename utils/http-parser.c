#include <strings.h>
#include "http-parser.h"

// zero out the request object
void initHttpRequest(HttpRequest *request)
{
    memset(request, 0, sizeof(HttpRequest));
    request->body = NULL;
    request->headerCount = 0;
}

// zero out the response object
void initHttpResponse(HttpResponse *response)
{
    memset(response, 0, sizeof(HttpResponse));
    response->body = NULL;
    response->headerCount = 0;
}

// create request object from given buffer
int parseHttpRequest(const char *requestBuffer, HttpRequest *request)
{
    char *savePtr = NULL;
    char *requestCopy = strdup(requestBuffer);

    initHttpRequest(request);

    // savePtr will now point to next section after \r\n
    char *line = strtok_r(requestCopy, "\r\n", &savePtr);

    // extract response code and status message
    if (!line || sscanf(line, "%7s %255s %15s", request->method, request->url, request->httpVersion) != 3)
    {
        free(requestCopy);
        return -1;
    }

    printf("request method: %s\nrequest url: %s\nrequest http version: %s\n", request->method, request->url, request->httpVersion);

    // allocating memory dynamically
    request->headers = (HttpHeader *)malloc(MAX_HEADERS * sizeof(HttpHeader));

    // now parsing headers
    while ((line = strtok_r(NULL, "\r\n", &savePtr)))
    {
        if (request->headerCount >= MAX_HEADERS)
            break;

        if (*line == '\0')
            break;

        // for splitting like Content-Type: text/html
        char *colon = strchr(line, ':');
        if (colon)
        {
            *colon = '\0'; // splitting key-value

            // will have ex. Content-Type
            request->headers[request->headerCount].key = strdup(line);

            // will have ex. text/html
            char *value = colon + 1;
            while (*value == ' ')
                value++;

            request->headers[request->headerCount].value = strdup(value);
            request->headerCount++;
        }
    }

    // now parsing body
    request->body = savePtr && *savePtr != '\0' ? strdup(savePtr) : NULL;
    printf("request-body: %s\n", request->body);
    free(requestCopy);
    return 0;
}

// create response object from given buffer
int parseHttpResponse(const char *responseBuffer, HttpResponse *response)
{
    // HTTP/1.1 200 OK\r\n
    // Content - Type : text / html\r\n
    // Content - Length : 27\r\n
    // Server : Apache\r\n\r\n
    // <html> Hello World !</ html>

    char *savePtr = NULL;
    char *responseCopy = strdup(responseBuffer);

    initHttpResponse(response);

    // savePtr will now point to next section after \r\n
    char *line = strtok_r(responseCopy, "\r\n", &savePtr);

    // parsing status code and status message
    if (!line || sscanf(line, "HTTP/%15s %d %[^\r\n]", response->httpVersion, &response->statusCode, response->statusMessage) != 3)
    {
        free(responseCopy);
        return -1;
    }

    // dynamically allocate memory for headers
    response->headers = (HttpHeader *)malloc(MAX_HEADERS * sizeof(HttpHeader));

    // now parsing headers
    while ((line = strtok_r(NULL, "\r\n", &savePtr)))
    {
        if (response->headerCount >= MAX_HEADERS)
            break;

        // check if we have reached \r\n\r\n meaning body can start now
        if (*line == '\0')
            break;

        // for splitting like Content-Type: text/html
        char *colon = strchr(line, ':');
        if (colon)
        {
            *colon = '\0'; // splitting key-value

            // will have ex. Content-Type
            response->headers[response->headerCount].key = strdup(line);

            // will have ex. text/html
            char *value = colon + 1;
            while (*value == ' ')
                value++;

            response->headers[response->headerCount].value = strdup(value);
            response->headerCount++;
        }
    }

    // now parsing body
    response->body = savePtr && *savePtr != '\0' ? strdup(savePtr) : NULL;

    free(responseCopy);

    return 0;
}

char *unparseHttpResponse(HttpResponse *response)
{
    // HTTP/1.1 200 OK\r\n
    // Content - Type : text / html\r\n
    // Content - Length : 27\r\n
    // <html> Hello World !</ html>

    char buffer[8196];
    int offset = 0;

    // adding
    // - http-version
    // - status-code
    // - status-message
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "HTTP/%s %d %s\r\n", response->httpVersion, response->statusCode, response->statusMessage);

    printf("http-version: %s\nstatus-code: %d\nstatus message: %s\n", response->httpVersion, response->statusCode, response->statusMessage);

    int contentLengthPresent = 0;

    printf("no of headers present : %d\n", response->headerCount);

    // adding headers
    for (int i = 0; i < response->headerCount; i++)
    {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);

        printf("%s: %s\n", response->headers[i].key, response->headers[i].value);

        if (strcasecmp(response->headers[i].key, "Content-Length") == 0)
            contentLengthPresent = 1;
    }

    // add content length if not present in header
    if (!contentLengthPresent && response->body)
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Content-Length: %zu\r\n", strlen(response->body));

    // end headers section
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\r\n");

    // add body if given
    if (response->body)
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", response->body);

    printf("response body: %s\n", response->body);

    char *responseBuffer = malloc(offset + 1);
    if (!responseBuffer)
        return NULL; // Handle allocation failure

    strncpy(responseBuffer, buffer, offset);
    responseBuffer[offset] = '\0'; // Ensure null termination

    return responseBuffer;
}

// free the dynamically allocated props of request object
void freeHttpRequest(HttpRequest *request)
{
    for (int i = 0; i < request->headerCount; i++)
    {
        free(request->headers[i].key);
        free(request->headers[i].value);
    }
    free(request->body);
    free(request->headers);
}

// free the dynamically allocated props of response object
void freeHttpResponse(HttpResponse *response)
{
    for (int i = 0; i < response->headerCount; i++)
    {
        free(response->headers[i].key);
        free(response->headers[i].value);
    }
    free(response->body);
    free(response->headers);
}