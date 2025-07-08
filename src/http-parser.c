#include "../include/http-parser.h"

HttpResponse *parse_http_response(const char *raw, size_t raw_len)
{
    if (!raw || raw_len == 0)
        return NULL;

    HttpResponse *res = calloc(1, sizeof(HttpResponse));
    if (!res)
        return NULL;

    res->contentLength = -1; // default if not specified

    // Step 1: Find header-body separator
    const char *header_end = NULL;
    size_t header_len = 0;

    // finding the headers end
    for (size_t i = 0; i + 3 < raw_len; ++i)
    {
        if (raw[i] == '\r' && raw[i + 1] == '\n' && raw[i + 2] == '\r' && raw[i + 3] == '\n')
        {
            header_end = raw + i + 4;
            header_len = i + 4;
            break;
        }
    }

    // invalid HTTP response
    if (!header_end || header_end < raw || header_end > raw + raw_len)
    {
        free(res);
        printf("no header end found, invalid response\n");
        return NULL;
    }

    // Step 2: Make a modifiable copy of the status+header section
    char *headers = malloc(header_len + 1);
    if (!headers)
    {
        free(res);
        return NULL;
    }

    memcpy(headers, raw, header_len);
    headers[header_len] = '\0'; // null-terminate for strtok_r

    // Step 3: Parse status line
    char *saveptr;
    char *line = strtok_r(headers, "\r\n", &saveptr);
    if (!line)
        goto fail;

    if (sscanf(line, "%15s %d %63[^\r\n]", res->httpVersion, &res->statusCode, res->statusMessage) != 3)
        goto fail;

    // Step 4: Parse headers line-by-line
    while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL)
    {
        while (*line == ' ')
            line++; // trim leading spaces

        if (strncasecmp(line, "Content-Type:", 13) == 0)
        {
            const char *val = line + 13;
            while (*val == ' ')
                val++;
            strncpy(res->contentType, val, sizeof(res->contentType) - 1);
        }
        else if (strncasecmp(line, "Content-Length:", 15) == 0)
        {
            const char *val = line + 15;
            while (*val == ' ')
                val++;
            res->contentLength = atoi(val);
        }
        else if (strncasecmp(line, "Transfer-Encoding:", 18) == 0)
        {
            const char *val = line + 18;
            while (*val == ' ')
                val++;
            if (strncasecmp(val, "chunked", 7) == 0)
                res->isChunked = 1;
        }
        else if (strncasecmp(line, "Location:", 9) == 0)
        {
            const char *val = line + 9;
            while (*val == ' ')
                val++;
            strncpy(res->location, val, sizeof(res->location) - 1);
        }
    }

    // Step 5: Handle body
    size_t body_len = raw_len - (header_end - raw);
    res->body = calloc(body_len + 1, 1);
    if (!res->body)
        goto fail;

    memcpy(res->body, header_end, body_len);
    res->body[body_len] = '\0';
    res->bodyLength = body_len;

    free(headers);
    return res;

fail:
    free_http_response(res);
    free(headers);
    free(res);
    return NULL;
}

HttpRequest *parse_http_request(const char *rawRequest, size_t raw_len)
{
    if (!rawRequest || rawRequest[0] == '\0')
        return NULL;

    // allocated space for request object
    HttpRequest *req = calloc(1, sizeof(HttpRequest));
    if (!req)
        return NULL;

    size_t status_line_len = 0;

    // finding the status line end
    for (size_t i = 0; i + 2 < raw_len; ++i)
    {
        if (rawRequest[i] == '\r' && rawRequest[i + 1] == '\n')
        {
            status_line_len = i + 2;
            printf("got status line end and status line length\n");
            break;
        }
    }

    // allocate space for the status line
    char *line = calloc(status_line_len + 1, 1);
    if (!line)
    {
        free(req);
        return NULL;
    }

    // copy the whole status line
    memcpy(line, rawRequest, status_line_len);
    line[status_line_len] = '\0';

    // now splitting the status line
    char *saveptr = 0;
    char *method = strtok_r(line, " ", &saveptr);
    char *fullPath = strtok_r(NULL, " ", &saveptr);
    char *version = strtok_r(NULL, "\r\n", &saveptr);

    if (!method || !fullPath || !version)
    {
        printf("invalid request received\n");
        free(line);
        free(req);
        return NULL;
    }

    strncpy(req->method, method, sizeof(req->method) - 1);
    strncpy(req->http_version, version, sizeof(req->http_version) - 1);

    // Split path and query
    char *qmark = strchr(fullPath, '?');
    if (qmark)
    {
        size_t pathLen = qmark - fullPath;
        strncpy(req->path, fullPath, pathLen);
        req->path[pathLen] = '\0';

        char *url_start = strstr(qmark, "url=");
        if (url_start)
        {
            url_start += 4;
            req->query = strdup(url_start);
        }
        else
        {
            req->query = NULL;
            printf("got invalid query params: %s\n", qmark);
        }
    }
    else
    {
        strncpy(req->path, fullPath, sizeof(req->path) - 1);
        req->query = NULL;
    }

    free(line);
    return req;
}

void free_http_response(HttpResponse *res)
{
    if (res && res->body)
    {
        free(res->body);
        res->body = NULL;
    }
}

void free_http_request(HttpRequest *req)
{
    if (req && req->query)
    {
        free(req->query);
        req->query = NULL;
    }
}

void init_http_response(HttpResponse *res)
{
    memset(res, 0, sizeof(HttpResponse));
    res->body = NULL;
}

void init_http_request(HttpRequest *req)
{
    memset(req, 0, sizeof(HttpRequest));
    req->query = NULL;
}