#include "../include/http-request-response.h"

int send_http_request(int sockfd, SSL *ssl, const char *host, const char *path)
{
    char request[2048];
    int len = snprintf(
        request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\n"
        "Accept-Language: en-US,en;q=0.9\r\n"
        "Host: %s\r\n"
        "Accept: */*\r\n"
        "Accept-Encoding: identity\r\n"
        "Referer: https://%s\r\n"
        "Connection: close\r\n"
        "\r\n",
        path, host, host);

    if ((size_t)len >= sizeof(request))
    {
        fprintf(stderr, "Request too long!\n");
        return -1;
    }

    int sent = 0;
    while (sent < len)
    {
        int n;
        if (ssl)
        {
            n = SSL_write(ssl, request + sent, len - sent);
        }
        else
        {
            n = send(sockfd, request + sent, len - sent, 0);
        }

        if (n <= 0)
        {
            perror("send_http_request");
            return -1;
        }

        sent += n;
    }

    return sent;
}

char *recv_response(int sockfd, SSL *ssl, size_t *out_len)
{
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = calloc(buffer_size, 1);
    if (!buffer)
        return NULL;

    size_t total_read = 0;
    int n;

    while (1)
    {
        // Resize buffer if full
        if (total_read + 4096 > buffer_size)
        {
            buffer_size *= 2;
            char *new_buf = realloc(buffer, buffer_size);
            if (!new_buf)
            {
                free(buffer);
                return NULL;
            }
            buffer = new_buf;
        }

        // Read data into buffer
        if (ssl)
        {
             n = SSL_read(ssl, buffer + total_read, buffer_size - total_read);
        }
        else
        {
            n = recv(sockfd, buffer + total_read, buffer_size - total_read, 0);
        }

        if (n < 0)
        {
            perror("recv_response");
            free(buffer);
            return NULL;
        }

        if (n == 0)
            break; // connection closed

        total_read += n;
    }

    *out_len = total_read;
    return buffer;
}

int send_http_response(int sockfd, char *body, size_t body_length, char *content_type)
{
    char response[512] = {0};

    int len = snprintf(
        response,
        sizeof(response) - 1,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        content_type,
        body_length);

    // sending status line and headers
    int bytes_sent = 0;
    if ((bytes_sent = send(sockfd, response, len, 0)) < 0)
    {
        printf("failed to send headers to client\n");
        return -1;
    }

    if ((bytes_sent = send(sockfd, body, body_length, 0)) < 0)
    {
        printf("failed to send body to client\n");
        return -1;
    }

    return bytes_sent;
}

char *recv_request(int sockfd, size_t *out_len)
{
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = calloc(buffer_size, 1);
    if (!buffer)
        return NULL;

    size_t total_read = 0;

    while (1)
    {
        // Resize buffer if full
        if (total_read + 4096 > buffer_size)
        {
            buffer_size *= 2;
            char *new_buf = realloc(buffer, buffer_size);
            if (!new_buf)
            {
                free(buffer);
                return NULL;
            }
            buffer = new_buf;
        }

        // Read data into buffer
        int n = recv(sockfd, buffer + total_read, buffer_size - total_read - 1, 0);

        if (n < 0)
        {
            perror("recv_response");
            free(buffer);
            return NULL;
        }

        if (n == 0)
            break; // connection closed

        total_read += n;
        buffer[total_read] = '\0';

        if (strstr(buffer, "\r\n\r\n") != NULL)
            break;
    }

    *out_len = total_read;
    return buffer;
}
