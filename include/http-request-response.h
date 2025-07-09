#ifndef HTTP_REQUEST_RESPONSE_H
#define HTTP_REQUEST_RESPONSE_H

#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#define INITIAL_BUFFER_SIZE 8192

enum CUSTOM_ERROR_CODE
{
    SERVCONNFAIL = 2,   
    SERVREQFAIL = 4,
    SERVRESFAIL = 8,
    BADSERVRES = 16,
    REDIRERR = 32,
    CLNTREQFAIL = 64,
    BADCLNTREQ = 128,
    MISQRYPRM = 256,
    INTRSERVERR = 512,
    BLCKDSITEERR = 1024,
};

int send_http_request(int sockfd, SSL *ssl, const char *host, const char *path);

char *recv_response(int sockfd, SSL *ssl, size_t *out_len);

int send_http_response(int sockfd, char *data, size_t data_length, char *content_type);

char *recv_request(int sockfd, size_t *out_len);

// sends error message, returns size of the message
int send_error_message(int fd, int statusCode, const char *statusMessage, const char *body);

// handle sending specific errors to clients
void handle_sending_error(int cfd, int error_code);

#endif