#ifndef HTTP_REQUEST_RESPONSE_H
#define HTTP_REQUEST_RESPONSE_H

#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h> 
#include "fetch.h"

int send_http_request(int sockfd,SSL* ssl,const ParsedURL* parsed);

char *recv_response(int sockfd, SSL *ssl, size_t *out_len);

#endif