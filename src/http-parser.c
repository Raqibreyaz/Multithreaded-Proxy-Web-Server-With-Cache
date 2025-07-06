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

    for (size_t i = 0; i + 3 < raw_len; ++i)
    {
        if (raw[i] == '\r' && raw[i + 1] == '\n' && raw[i + 2] == '\r' && raw[i + 3] == '\n')
        {
            header_end = raw + i + 4;
            header_len = i + 4;
            break;
        }
    }

    if (!header_end)
    {
        free(res);
        return NULL; // invalid HTTP response
    }

    // Step 2: Make a modifiable copy of the header section
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
    res->body = malloc(body_len);
    if (!res->body)
        goto fail;

    memcpy(res->body, header_end, body_len);
    res->bodyLength = body_len;

    free(headers);
    return res;

fail:
    free(headers);
    free(res);
    return NULL;
}
