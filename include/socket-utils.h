#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int open_connection(const char *host, const char *port);
SSL* ssl_wrap(int sockfd, const char *hostname, SSL_CTX **out_ctx);// SSL handshake + verification

#endif