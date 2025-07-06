#include "../include/socket-utils.h"
#include "../include/fetch.h"

int open_connection(const char *host, const char *port)
{
    struct addrinfo hints, *res, *p;
    int sockfd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    if (getaddrinfo(host, port, &hints, &res) != 0)
    {
        perror("getaddrinfo");
        return -1;
    }

    // Loop through all results and connect to the first we can
    for (p = res; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            sockfd = -1;
            continue;
        }

        break; // success
    }

    freeaddrinfo(res);

    if (sockfd == -1)
    {
        fprintf(stderr, "Failed to connect to %s:%s\n", host, port);
    }

    return sockfd;
}

// Global one-time init flag
static int ssl_initialized = 0;

SSL* ssl_wrap(int sockfd, const char *hostname, SSL_CTX **out_ctx) {
    if (!ssl_initialized) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        ssl_initialized = 1;
    }

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_set_fd(ssl, sockfd);
    SSL_set_tlsext_host_name(ssl, hostname);  // SNI for modern HTTPS

    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return NULL;
    }

    *out_ctx = ctx;
    return ssl;
}
