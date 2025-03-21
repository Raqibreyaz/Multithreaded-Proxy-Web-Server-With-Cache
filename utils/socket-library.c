#include "socket-library.h"

ssize_t sendMessage(int fd, int flags, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    size_t size = vsnprintf(NULL, 0, format, args) + 1; // +1 for \0
    if (size <= 0)
    {
        va_end(args);
        return -1;
    }

    ssize_t bytes_sent = -1;
    char *buffer = (char *)malloc(size); // Dynamic allocation for consistency

    if (buffer == NULL)
    {
        va_end(args);
        return -1; // Allocation failure
    }

    va_start(args, format); // Restart va_list for vsnprintf
    if (vsnprintf(buffer, size, format, args) < 0)
    {
        free(buffer);
        va_end(args);
        return -1;
    }
    va_end(args); // va_end after vsnprintf

    bytes_sent = send(fd, buffer, size - 1, flags); // size -1 to prevent sending null terminator

    free(buffer);
    return bytes_sent;
}

// handle receive via TCP
ssize_t recvMessage(int fd, int flags, char *buffer, size_t bufferSize)
{
    // receive data
    ssize_t bytes_read = recv(fd, buffer, bufferSize - 1, flags);

    // make the buffer null terminated
    if (bytes_read >= 0)
        buffer[bytes_read] = '\0';

    // return the final read bytes
    return bytes_read;
}

ssize_t recvAllData(int fd, char *buffer, size_t bufferSize, int flags)
{
    ssize_t usedSize = bufferSize - 1;

    ssize_t totalReceivedBytes = 0, receivedBytes = 0;

    int headersCompleted = 0;
    size_t bodyLength = 0;

    // receive all the data at max bufferSize-1
    while (totalReceivedBytes < usedSize)
    {
        receivedBytes = recv(fd, buffer + totalReceivedBytes, usedSize - totalReceivedBytes, flags);

        if (receivedBytes <= 0)
            break;

        totalReceivedBytes += receivedBytes;

        // when headers are completed then check if body is available
        if (!headersCompleted && strstr(buffer, "\r\n\r\n"))
        {
            // headers are now completed
            headersCompleted = 1;

            // now check if content length is present
            char *contentLengthPtr = strstr(buffer, "Content-Length:");

            // if content length is present then extract the length of content
            if (contentLengthPtr)
            {
                sscanf(contentLengthPtr, "Content-Length: %zu\r\n", &bodyLength);
                printf("Detected content-length: %zu\n", bodyLength);
            }

            // if no content present then end the loop
            if (bodyLength == 0)
                break;
        }

        // when headers are completed and content length exists check if we received full content of body
        if (headersCompleted && bodyLength > 0)
        {
            // calculate the headers szie
            ssize_t headersSize = strstr(buffer, "\r\n\r\n") - buffer + 4;

            // calculate the amount of content of body we received
            ssize_t bodyReceived = totalReceivedBytes - headersSize;

            // if we received all the data then end the loop
            if (bodyReceived >= bodyLength)
                break;
        }
    }

    if (receivedBytes < 0)
    {
        close(fd);
        printf("failed to receive data from client\n");
    }

    // last byte will be terminator
    buffer[totalReceivedBytes] = '\0';

    // when last read failed then return error
    return receivedBytes < 0 ? receivedBytes : totalReceivedBytes;
};

// send message via specifically udp
ssize_t sendMessagePacket(int fd, int flags, struct sockaddr *addr, socklen_t addrLen, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    size_t size = vsnprintf(NULL, 0, format, args) + 1; // +1 for \0
    if (size <= 0)
    {
        va_end(args);
        return -1;
    }

    ssize_t bytes_sent = -1;
    char *buffer = (char *)malloc(size); // Dynamic allocation for consistency

    if (buffer == NULL)
    {
        va_end(args);
        return -1; // Allocation failure
    }

    va_start(args, format); // Restart va_list for vsnprintf
    if (vsnprintf(buffer, size, format, args) < 0)
    {
        free(buffer);
        va_end(args);
        return -1;
    }
    va_end(args); // va_end after vsnprintf

    bytes_sent = sendto(fd, buffer, size - 1, flags, addr, addrLen); // size -1 to prevent sending null terminator

    free(buffer);
    return bytes_sent;
}

// handle receiving via specifically udp
ssize_t recvMessagePacket(int fd, char *buffer, size_t bufferSize, int flags, struct sockaddr *addr, socklen_t *addrLen)
{
    ssize_t bytes_received = recvfrom(fd, buffer, bufferSize, flags, addr, addrLen);

    if (bytes_received >= 0)
        buffer[bytes_received] = '\0';
    return bytes_received;
}

// create a socket
int createSocket(int domain, int type, int protocol)
{
    int sfd;
    if ((sfd = socket(domain, type, protocol)) == -1)
        fatal("socket");
    return sfd;
}

// bind the socket with the provided address
void bindWithAddress(int sfd, struct sockaddr *addr, socklen_t addrLen)
{
    if (bind(sfd, addr, addrLen) == -1)
        fatalWithClose(sfd, "bind");
}

// connect to local/remote server
int connectWithServer(int sfd, struct sockaddr *addr, socklen_t addrLen, int exitOnFail)
{
    int status = connect(sfd, addr, addrLen);
    if (exitOnFail)
        fatalWithClose(sfd, "connect");
    return status;
}

int createConnection(
    int domain,
    int type,
    const char *hostname,
    const char *service,
    struct sockaddr_storage *server_addr)
{
    // create a socket for connection
    int cfd = createSocket(domain, type, 0);

    // for resolving ip adresses
    struct addrinfo hints, *res, *temp;

    // initialize by 0 for dns lookup
    memset(&hints, 0, sizeof(struct addrinfo));

    // set whether ipv4/ipv6
    hints.ai_family = domain;

    // set the default protocol
    hints.ai_protocol = 0;

    // set whether it is a udp or tcp socket
    hints.ai_socktype = type;

    // resolving ip
    int status;
    if ((status = getaddrinfo(hostname, service, &hints, &res)) != 0)
    {
        exitAndCloseWithMessage(cfd, gai_strerror(status));
    }

    // traverse the ip list
    for (temp = res; temp != NULL; temp = temp->ai_next)
    {
        void *addr = NULL;

        // handle ipv4 case
        if (temp->ai_family == AF_INET)
        {
            // typecasting it from generic socket to ipv4 for extracting ipv4 address
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)temp->ai_addr;

            // getting the binary address
            addr = &(ipv4->sin_addr);
        }

        //   handle ipv6 case
        else
        {
            // typecasting it from generic socket to ipv6 for extracting ipv6 address
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)temp->ai_addr;

            // getting the binary address
            addr = &(ipv6->sin6_addr);
        }

        // take the socket address
        memcpy(server_addr, temp->ai_addr, temp->ai_addrlen);

        // if tcp then try to conenct with server
        if (
            type == SOCK_STREAM &&
            connectWithServer(cfd, temp->ai_addr, temp->ai_addrlen, 0) == -1)
            continue;

        printf("connected with %s\n", hostname);

        // stop the loop
        break;
    }

    // freeing the ip list
    freeaddrinfo(res);

    return cfd;
}

// listen for specified no of clients
void listenToClient(int sfd, int nClients)
{
    if (listen(sfd, nClients) == -1)
        fatalWithClose(sfd, "listen");
}

// accept client connection
int acceptClient(int sfd, struct sockaddr *__restrict__ addr, socklen_t *__restrict__ addrLen)
{
    int cfd;
    if ((cfd = accept(sfd, addr, addrLen)) == -1)
        printf("failed to accept connection\n");
    return cfd;
}

// Create server that supports both IPv4 and IPv6
int createServer(
    int domain,
    int type,
    int port,
    int backlog,
    const char *ip,
    struct sockaddr_storage *server_addr)
{
    // create a socket
    int sfd = createSocket(domain, type, 0);
    socklen_t addrLen;

    // initialize the address with 0
    memset(server_addr, 0, sizeof(*server_addr));

    // handle ipv4 case
    if (domain == AF_INET)
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)server_addr;
        addrLen = sizeof(struct sockaddr_in);
        addr4->sin_family = domain;
        addr4->sin_port = htons(port);

        convertToBinaryIP(sfd, domain, ip, (struct in_addr *)&addr4->sin_addr);
    }

    // handle ipv6 case
    else if (domain == AF_INET6)
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)server_addr;
        addrLen = sizeof(struct sockaddr_in6);
        addr6->sin6_family = domain;
        addr6->sin6_port = htons(port);

        convertToBinaryIP(sfd, domain, ip, (struct in6_addr *)&addr6->sin6_addr);
    }

    // throw error when unknow domain encountered
    else
        exitAndCloseWithMessage(sfd, "Unsupported Domain\n");

    // bind the socket with the address
    bindWithAddress(sfd, (struct sockaddr *)server_addr, addrLen);

    // listen is required only for TCP
    if (type == SOCK_STREAM)
    {
        listenToClient(sfd, backlog);
        printf("server is listening on port %d...\n", port);
    }

    return sfd;
}

int sendWelcomeMessage(int fd, HttpResponse *response)
{
    initHttpResponse(response);

    response->statusCode = 200;
    strcpy(response->httpVersion, "1.1");
    strcpy(response->statusMessage, "OK");

    char body[] = "<html><body><h1>Welcome to Proxy!</h1></body></html>";

    response->headerCount = 3;
    response->headers = malloc(response->headerCount * sizeof(HttpHeader));

    response->headers[0].key =
        strdup("Content-Type");

    response->headers[0].value =
        strdup("text/html");

    response->headers[1].key =
        strdup("Content-Length");

    response->headers[1].value =
        convertNumberToString(strlen(body));

    response->headers[2].key =
        strdup("Connection");

    response->headers[2].value =
        strdup("close");

    response->body = strdup(body);

    // unparse the response object into raw string
    char *responseBuffer = unparseHttpResponse(response);

    printf("response: %s\n", responseBuffer);

    ssize_t bytes_sent = sendMessage(fd, 0, responseBuffer);
    // ssize_t bytes_sent = send(fd, responseBuffer, strlen(responseBuffer), 0);

    if (bytes_sent > 0)
    {
        printf("response sent\n");
        free(responseBuffer);
    }
    else
        printf("failed to send response\n");

    freeHttpResponse(response);

    return bytes_sent;
}