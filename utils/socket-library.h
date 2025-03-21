#ifndef SOCKET_LIBRARY_H
#define SOCKET_LIBRARY_H

#define _POSIX_C_SOURCE 200809
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include "http-parser.h"
#include "custom-utilities.h"

#define REQUEST_BUFFER_SIZE 8196
#define RESPONSE_BUFFER_SIZE 8196

// send message to client/server
ssize_t sendMessage(int fd, int flags, const char *format, ...);

// receive data from client/server
ssize_t recvMessage(int fd, int flags, char *buffer, size_t bufferSize);

// receieve all the data in the buffer at max its size
ssize_t recvAllData(int fd, char *buffer, size_t bufferSize, int flags);

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
    struct sockaddr_storage *server_addr);

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

int sendWelcomeMessage(int fd, HttpResponse *response);

#endif