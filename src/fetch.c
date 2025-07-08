#include "../include/fetch.h"

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
struct HttpResponse *fetch_url(const char *url, int max_redirects)
{
    int sockfd = -1;
    ParsedURL parsed;
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    char *raw = NULL;
    struct HttpResponse *res = NULL;

    if (max_redirects <= 0)
    {
        fprintf(stderr, "Too many redirects\n");
        return NULL;
    }

    // Parse URL
    if (parse_url(url, &parsed) == 0)
    {
        fprintf(stderr, "Invalid URL: %s\n", url);
        return NULL;
    }

    // Connect to remote server
    sockfd = open_connection(parsed.host, parsed.port);
    if (sockfd < 0)
        goto cleanup;

    // Perform SSL/TLS handshake if needed
    if (strcmp(parsed.scheme, "https") == 0)
    {
        ssl = ssl_wrap(sockfd, parsed.host, &ctx);
        if (!ssl)
            goto cleanup;
    }

    // Send HTTP request
    if (send_http_request(sockfd, ssl, parsed.host, parsed.path) < 0)
        goto cleanup;

    // Receive raw response
    size_t raw_len;
    raw = recv_response(sockfd, ssl, &raw_len);
    if (!raw)
        goto cleanup;

    // Parse response
    res = parse_http_response(raw, raw_len);
    free(raw);
    raw = NULL;

    if (!res)
        goto cleanup;

    // Handle Redirects (e.g., 301, 302)
    if (res->isRedirect || (res->statusCode >= 300 && res->statusCode < 400))
    {
        char redirect_url[URL_MAX_LEN] = {0};

        if (res->location[0] == '/')
        {
            // Handle relative redirect
            snprintf(redirect_url, sizeof(redirect_url), "%s://%s%s",
                     parsed.scheme, parsed.host, res->location);
        }
        else
        {
            // Absolute redirect
            strncpy(redirect_url, res->location, sizeof(redirect_url) - 1);
        }

        // Prevent redirect loop
        if (urls_are_equivalent(url, redirect_url))
        {
            fprintf(stderr, "Redirect loop detected to: %s\n", redirect_url);
            goto cleanup;
        }

        fprintf(stderr, "Redirecting to: %s\n", redirect_url);

        // Free resources and recurse
        free_http_response(res);
        free(res);
        close(sockfd);
        SSL_CTX_free(ctx);
        SSL_free(ssl);
        ctx = NULL;
        ssl = NULL;
        sockfd = -1;

        res = fetch_url(redirect_url, max_redirects - 1);
    }

    if (sockfd != -1)
        close(sockfd);
    if (ctx)
        SSL_CTX_free(ctx);
    if (ssl)
        SSL_free(ssl);

    return res;

cleanup:
    if (sockfd != -1)
        close(sockfd);
    if (ctx)
        SSL_CTX_free(ctx);
    if (ssl)
        SSL_free(ssl);
    if (raw)
        free(raw);
    if (res)
    {
        free_http_response(res);
        free(res);
    }
    return NULL;
}
struct HttpResponse *fetch_cache_or_url(CacheLRU *cache, const char *url, int max_redirects)
{
    // when cache present then move the node to head and return cache response
    if (lru_contains(cache, url))
    {
        // move to head
        lru_touch(cache, url);

        // initializing vars
        char *cache_filename = NULL;
        char *data = NULL;
        HttpResponse *res = NULL;
        char content_type[128] = {0};
        size_t data_len = 0;

        // get the sanitized filename
        cache_filename = get_cache_filename(url);

        // return cache response
        data = read_cache_file(cache_filename, content_type, &data_len);

        if (!data)
            goto catch;

        // allocate space for res object
        res = (HttpResponse *)calloc(1, sizeof(HttpResponse));
        if (!res)
        {
            printf("failed to allocate space for response object\n");
            goto catch;
        }

        // initialize the res
        res->statusCode = 200;
        strcpy(res->statusMessage, "OK");
        strcpy(res->httpVersion, "HTTP/1.1");
        strcpy(res->contentType, content_type);
        res->bodyLength = data_len;
        res->contentLength = data_len;
        res->isChunked = 0;
        res->isRedirect = 0;
        res->body = data;
        data = NULL;

        printf("serving from cache\n");

        free(cache_filename);
        return res;

    // will run for errors
    catch:
        if (cache_filename)
            free(cache_filename);
        if (data)
            free(data);
        if (res)
            free(res);
        return NULL;
    }

    printf("requesting remote server for response\n");

    // fetch from remote server and cache the response
    HttpResponse *res = fetch_url(url, max_redirects);

    // rewrite html links for our proxy

    lru_insert(cache, url, res->body, res->bodyLength, res->contentType);

    return res;
}