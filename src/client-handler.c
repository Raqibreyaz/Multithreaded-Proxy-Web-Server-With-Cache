#include "../include/client-handler.h"

void handle_client(int client_fd, CacheLRU *cache, char *blocked_sites[], int n_of_b_sites)
{
    char *raw_request = NULL;
    char *data = NULL;
    HttpRequest *req = NULL;
    HttpResponse *res = NULL;

    size_t raw_len = 0;
    raw_request = recv_request(client_fd, &raw_len);
    if (!raw_request)
    {
        printf("failed to receive request from client\n");
        return;
    }

    req = parse_http_request(raw_request, raw_len);
    if (!req)
    {
        printf("request parsing failed\n");
        goto cleanup;
    }

    // Dispatch based on path:
    if (req->query == NULL && strcmp(req->path, "/") == 0)
    {
        size_t data_size = 0;
        data = read_file("static/search.html", &data_size);
        if (!data)
        {
            goto cleanup;
        }

        if (send_http_response(client_fd, data, data_size, "text/html") <= 0)
        {
            printf("failed to send response to client\n");
            goto cleanup;
        }
    }
    else if (strcmp(req->path, "/favicon.ico") == 0)
    {
        size_t data_size = 0;
        data = read_file("static/favicon.ico", &data_size);
        if (!data)
        {
            goto cleanup;
        }

        if (send_http_response(client_fd, data, data_size, "image/x-icon") <= 0)
        {
            printf("failed to send response to client\n");
            goto cleanup;
        }
    }
    else if (req->query)
    {
        ParsedURL parsed_url;
        parse_url(req->query, &parsed_url);

        // close the connection if the site is blocked
        if (is_site_blocked(blocked_sites, n_of_b_sites, parsed_url.host))
        {
            printf("closing connection as blocked site is requested\n");
            goto cleanup;
        }

        res = fetch_cache_or_url(cache, req->query, MAX_REDIRECTS_ALLOWED);

        if (send_http_response(client_fd, res->body, res->bodyLength, res->contentType) <= 0)
        {
            printf("failed to respond data to client\n");
            goto cleanup;
        }
    }

    goto cleanup;

cleanup:
    if (req)
    {
        free_http_request(req);
        free(req);
    }
    if (res)
    {
        free_http_response(res);
        free(res);
    }
    if (raw_request)
        free(raw_request);
    if (client_fd != -1)
        close(client_fd);
}
