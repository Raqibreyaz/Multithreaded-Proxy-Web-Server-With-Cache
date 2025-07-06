#include "../include/http-request-response.h"
#include "../include/http-parser.h"
#include "../include/fetch.h"
#include "../include/cache.h"
#include "../include/socket-utils.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// http://localhost:4000/favicon.ico
// scheme->http
// host->localhost
// port->4000
// path->/favicon.ico
int parse_url(const char *url, ParsedURL *out)
{
    if (!url || !out)
        return 0;

    size_t url_size = strlen(url);

    // Temporary working buffer
    char temp[url_size + 1];
    strcpy(temp, url);
    temp[url_size] = '\0';

    // Set defaults
    strcpy(out->scheme, "http");
    strcpy(out->port, "80");
    strcpy(out->path, "/");

    char *ptr = temp;
    char *scheme_end = strstr(ptr, "://");
    if (scheme_end)
    {
        *scheme_end = '\0';
        strncpy(out->scheme, ptr, sizeof(out->scheme) - 1);
        ptr = scheme_end + 3;

        if (strcmp(out->scheme, "https") == 0)
        {
            strcpy(out->port, "443");
        }
    }

    // Now ptr points to [host[:port]][/path]
    char *path_start = strchr(ptr, '/');
    if (path_start)
    {
        strncpy(out->path, path_start, sizeof(out->path) - 1);
        *path_start = '\0'; // Temporarily split host[:port]
    }

    // ptr now contains host[:port]
    char *colon = strchr(ptr, ':');
    if (colon)
    {
        *colon = '\0';
        strncpy(out->host, ptr, sizeof(out->host) - 1);
        strncpy(out->port, colon + 1, sizeof(out->port) - 1);
    }
    else
    {
        strncpy(out->host, ptr, sizeof(out->host) - 1);
    }

    return 1; // success
}

HttpResponse *fetch_url(const char *url, int max_redirects)
{

    if (max_redirects <= 0)
    {
        fprintf(stderr, "Too many redirects\n");
        return NULL;
    }

    ParsedURL parsed;
    if (parse_url(url, &parsed) != 0)
    {
        fprintf(stderr, "Invalid URL: %s\n", url);
        return NULL;
    }

    int sockfd = open_connection(parsed.host, parsed.port);
    if (sockfd < 0)
        return NULL;

    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;

    if (strcmp(parsed.scheme, "https") == 0)
    {
        ssl = ssl_wrap(sockfd, parsed.host, &ctx);
        if (!ssl)
        {
            close(sockfd);
            return NULL;
        }
    }

    if (send_http_request(sockfd, ssl, &parsed) < 0)
    {
        if (ssl)
            SSL_free(ssl);
        if (ctx)
            SSL_CTX_free(ctx);
        close(sockfd);
        return NULL;
    }

    size_t raw_len;
    char *raw = recv_response(sockfd, ssl, &raw_len);

    if (ssl)
        SSL_free(ssl);
    if (ctx)
        SSL_CTX_free(ctx);
    close(sockfd);

    if (!raw)
        return NULL;

    HttpResponse *res = parse_http_response(raw, raw_len);
    free(raw);

    if (!res)
        return NULL;

    // 🔁 Redirect handling
    if (res->isRedirect || (res->statusCode >= 300 && res->statusCode < 400 &&
                            res->body && res->bodyLength > 0))
    {
        // Try to extract Location from body if not parsed
        // (this should only be fallback)
        if (strlen(res->location) == 0)
        {
        }

        return fetch_url(res->location, max_redirects - 1);
    }

    if (res->statusCode >= 300 && res->statusCode < 400)
        fprintf(stderr, "Redirected to: %s\n", res->location);

    return res;
}
