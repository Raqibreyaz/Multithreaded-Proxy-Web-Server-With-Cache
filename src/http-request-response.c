#include "../include/http-request-response.h"

int send_http_request(int sockfd, SSL *ssl, const ParsedURL *parsed) {
    char request[2048];
    int len = snprintf(
        request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "Accept-Encoding: identity\r\n"
        "\r\n",
        parsed->path, parsed->host
    );

    if (len >= sizeof(request)) {
        fprintf(stderr, "Request too long!\n");
        return -1;
    }

    int sent = 0;
    while (sent < len) {
        int n;
        if (ssl) {
            n = SSL_write(ssl, request + sent, len - sent);
        } else {
            n = send(sockfd, request + sent, len - sent, 0);
        }

        if (n <= 0) {
            perror("send_http_request");
            return -1;
        }

        sent += n;
    }

    return 0;
}

char* recv_response(int sockfd, SSL *ssl, size_t *out_len) {
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(buffer_size);
    if (!buffer) return NULL;

    size_t total_read = 0;
    int n;

    while (1) {
        // Resize buffer if full
        if (total_read + 4096 > buffer_size) {
            buffer_size *= 2;
            char *new_buf = realloc(buffer, buffer_size);
            if (!new_buf) {
                free(buffer);
                return NULL;
            }
            buffer = new_buf;
        }

        // Read data into buffer
        if (ssl) {
            n = SSL_read(ssl, buffer + total_read, buffer_size - total_read);
        } else {
            n = recv(sockfd, buffer + total_read, buffer_size - total_read, 0);
        }

        if (n < 0) {
            perror("recv_response");
            free(buffer);
            return NULL;
        }

        if (n == 0) break; // connection closed

        total_read += n;
    }

    *out_len = total_read;
    return buffer;
}

