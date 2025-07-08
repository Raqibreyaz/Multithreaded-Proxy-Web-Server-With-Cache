#ifndef HTTP_REQUEST_RESPONSE_H
#define HTTP_REQUEST_RESPONSE_H

#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#define INITIAL_BUFFER_SIZE 8192

int send_http_request(int sockfd, SSL *ssl, const char *host, const char *path);

char *recv_response(int sockfd, SSL *ssl, size_t *out_len);

int send_http_response(int sockfd, char *data, size_t data_length, char *content_type);

char *recv_request(int sockfd, size_t *out_len);

#endif