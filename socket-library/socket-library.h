#ifndef SOCKET_LIBRARY_H
#define SOCKET_LIBRARY_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "../http-parser/http-parser.h"
#include "../utils/custom-utilities.h"
#include "../cache-list/cache-list.h"

#define REQUEST_BUFFER_SIZE 8196
#define RESPONSE_BUFFER_SIZE 16384

struct ClientHandlerArg
{
    int cfd;
    int *liveThreads;
    CacheNode **head;
    CacheNode **tail;
    pthread_mutex_t *mtx;
    pthread_cond_t *cond;
};

enum RESPONSE_ERROR_CODE
{
    SERVCONNFAIL = 1,
    SERVREQFAIL,
    SERVRESFAIL,
    BADSERVRES,
    REDIRERR
};

enum REQUEST_ERROR_CODE
{
    CLNTREQFAIL = 20,
    BADCLNTREQ,
    MISQRYPRM,
    INTRSERVERR
};

void exitCleanUp(int fd,
                 void *arg,
                 int *liveThreads,
                 HttpRequest *request,
                 HttpResponse *response,
                 pthread_mutex_t *mtx,
                 pthread_cond_t *cond,
                 char *requestBuffer,
                 char *responseBuffer);

// send message to client/server
ssize_t
sendMessage(int fd, int flags, const char *format, ...);

int send_message_securely(SSL *ssl, char *buffer, int buffer_size);

// receive data from client/server
ssize_t recvMessage(int fd, int flags, char *buffer, size_t bufferSize);

// receieve all the data in the buffer at max its size
ssize_t recvAllData(int fd, char *buffer, size_t bufferSize, int flags);

int recvAllDataSecurely(SSL *ssl, char *buffer, int buffer_size);

// send data via udp packets
ssize_t sendMessagePacket(int fd, int flags, struct sockaddr *addr, socklen_t addrLen, const char *format, ...);

// receive data via udp packet
ssize_t recvMessagePacket(int fd, char *buffer, size_t bufferSize, int flags, struct sockaddr *addr, socklen_t *addrLen);

// create a socket
int createSocket(int domain, int type, int protocol);

// bind the socket with the provided address
void bindWithAddress(int sfd, struct sockaddr *addr, socklen_t addrLen);

// connect to local/remote server
int connectWithServer(int sfd, struct sockaddr *addr, socklen_t addrLen, int exitOnFail);

// create a conenction with server
int createConnection(
    int domain,
    int type,
    const char *hostname,
    const char *service,
    struct sockaddr_storage *server_addr, int exitOnFail);

// listen for specified no of clients
void listenToClient(int sfd, int nClients);

// accept client connection
int acceptClient(int sfd, struct sockaddr *__restrict__ addr, socklen_t *__restrict__ addrLen);

// Create server that supports both IPv4 and IPv6
int createServer(
    int domain,
    int type,
    int port,
    int backlog,
    const char *ip,
    struct sockaddr_storage *server_addr);

int acceptRequest(int cfd, HttpRequest *request, char *requestBuffer);

int createResponse(int cfd, HttpResponse *response, HttpRequest *request, char *responseBuffer, char *requestBuffer);

void *handleClient(void *arg);

// send welcome message, returns size of the message
int sendWelcomeMessage(int fd, HttpResponse *response, const char *httpVersion);

// sends error message, returns size of the message
int sendErrorMessage(int fd, HttpResponse *response, const char *httpVersion, int statusCode, const char *statusMessage, const char *body);

void handleSendingError(HttpResponse *response, const char *http_version, int error_code, int cfd);

void check_ssl_error(SSL *ssl, int val, const char *context);

#endif  