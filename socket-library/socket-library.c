#include "socket-library.h"

void exitCleanUp(int fd,
                 void *arg,
                 int *liveThreads,
                 HttpRequest *request,
                 HttpResponse *response,
                 pthread_mutex_t *mtx,
                 pthread_cond_t *cond,
                 const char *requestBuffer,
                 const char *responseBuffer)
{
    // closing connection to client
    if (fd != -1)
        close(fd);

    // decrease no of threads
    if (liveThreads)
        (*liveThreads)--;

    // signaling other threads
    if (cond)
        pthread_cond_signal(cond);

    // unlocking mutex
    if (mtx)
        pthread_mutex_unlock(mtx);

    // freeing request
    if (request)
        free(request);

    // freeing response
    if (response)
    {
        freeHttpResponse(response);
        free(response);
    }

    // freeing requestbuffer
    if (requestBuffer)
        free(requestBuffer);

    // freeing response buffer
    if (responseBuffer)
        free(responseBuffer);

    // freeing the arg
    if (arg)
        free(arg);
}

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

// receive all the stream data from client/server
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
    // making the socket at that port reusable
    int yes = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

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
    struct sockaddr_storage *server_addr, int exitOnFail)
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
        if (exitOnFail == 0)
        {
            printf("failed to resolve ip\n");
            return -1;
        }

        exitAndCloseWithMessage(cfd, gai_strerror(status));
    }

    // traverse the ip list
    printf("ttraversing list to connect on ip\n");
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
            connectWithServer(cfd, temp->ai_addr, temp->ai_addrlen, exitOnFail) == -1)
            continue;

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
    else
        printf("server is available on port %d...\n", port);

    return sfd;
}

// will accept request from client and parse it to object
int acceptRequest(int cfd, HttpRequest *request, char *requestBuffer)
{
    // receive entire request from client
    if (recvAllData(cfd, requestBuffer, REQUEST_BUFFER_SIZE, 0) < 0)
        return CLNTREQFAIL;

    printf("received client request\n");

    // parse the http request
    int status = parseHttpRequest(request, requestBuffer);

    return status == -1 ? BADCLNTREQ : 1;
}

// will create a response for client using the params
int createResponse(int cfd, HttpResponse *response, HttpRequest *request, char *responseBuffer, char *requestBuffer)
{
    int response_status = 1;
    struct sockaddr_in serverAddr;

    // if no cached response available then forward the request to original server
    printf("\nno cached data available\n");

    // create raw request string from request object
    int requestBufferSize = unparseHttpRequest(request, requestBuffer, REQUEST_BUFFER_SIZE);

    // create connection to the remote server
    int sfd = createConnection(AF_INET, SOCK_STREAM, request->host, "https", (struct sockaddr_storage *)&serverAddr, 0);

    if (sfd == -1)
    {
        response_status = SERVCONNFAIL;
        goto finally;
    }

    printf("connected to remote server\n");

    // forward request to remote server
    if ((sendMessage(sfd, 0, "%s", requestBuffer)) == -1)
    {
        response_status = SERVREQFAIL;
        goto finally;
    }

    printf("requested to remote server\n");

    // receive data from remote server
    if ((recvAllData(sfd, responseBuffer, RESPONSE_BUFFER_SIZE, 0)) == -1)
    {
        response_status = SERVRESFAIL;
        goto finally;
    }

    printf("received response from remote server\n");

    if (parseHttpResponse(response, responseBuffer) == -1)
        response_status = BADSERVRES;

    goto finally;

finally:
    close(sfd);
    return response_status;
}

// will handle the receiving request to sending response to client
void *handleClient(void *arg)
{
    printf("inside handle client function\n");
    struct ClientHandlerArg *clientArg = (struct ClientHandlerArg *)arg;

    int cfd = clientArg->cfd;

    HttpRequest *request = (HttpRequest *)malloc(sizeof(HttpRequest));
    HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));
    char *requestBuffer = (char *)malloc(REQUEST_BUFFER_SIZE);
    char *responseBuffer = (char *)malloc(RESPONSE_BUFFER_SIZE);

    // if memory allocating failed then close connection and go back
    if (!request || !response || !requestBuffer || !responseBuffer)
    {
        close(cfd);
        free(arg);
        return NULL;
    }

    // accept request from client + handle requesting error
    int req_status = 1;
    if ((req_status = acceptRequest(cfd, request, requestBuffer)) != 1)
    {
        handleSendingError(response, "1.1", req_status, cfd);

        // freeing all the allocated resources
        exitCleanUp(cfd, arg, clientArg->liveThreads, request, response, NULL, clientArg->cond, requestBuffer, responseBuffer);

        return NULL;
    }

    // when our home page is requested then respond the home page
    if (strncmp(request->host, "localhost:", 10) == 0 && strcmp(request->path, "/") == 0)
    {
        printf("sending welcome message\n");
        sendWelcomeMessage(cfd, response, request->httpVersion);

        // freeing all the allocated resources
        exitCleanUp(cfd, arg, clientArg->liveThreads, request, response, NULL, clientArg->cond, requestBuffer, responseBuffer);

        return NULL;
    }

    // handle if no query url present
    if (strlen(request->query_url) == 0)
    {
        handleSendingError(response, request->httpVersion, MISQRYPRM, cfd);

        exitCleanUp(cfd, arg, clientArg->liveThreads, request, response, NULL, clientArg->cond, requestBuffer, responseBuffer);

        return NULL;
    }

    // lock so that no other thread could modify the list
    pthread_mutex_lock(clientArg->mtx);

    // check if there is a cached response available
    CacheNode *res = findCacheNode(*(clientArg->head), request->query_url);

    // if cached response found then use it
    if (res != NULL)
    {
        // prepare the full file path
        char file_path[1050];
        int bytes_written = snprintf(file_path, sizeof(file_path) - sizeof(".meta") - 1, "%s/%s", CACHE_DIR, res->url);
        file_path[bytes_written] = '\0';

        // read the cache file
        char *data = read_file(file_path);

        // prepare the data type file path
        bytes_written = snprintf(file_path + bytes_written, sizeof(file_path) - bytes_written - 1, ".meta");
        file_path[bytes_written] = '\0';

        // read the data type file
        char *data_type = read_file(file_path);

        response->statusCode = 200;
        strcpy(response->statusMessage, "Successful");
        strcpy(response->httpVersion, request->httpVersion);
        strcpy(response->contentType, data_type);
        response->contentLength = strlen(data);
        response->isChunked = 0;
        response->isRedirect = 0;
        response->body = data;

        // freeing the allocated space for the data type
        free(data_type);

        // move the node to head as it is recently used
        CacheNode *newTail = moveToHead(*(clientArg->head), res);

        // update the new head
        *(clientArg->head) = res;

        // update the tail if modified
        if (newTail)
            *(clientArg->tail) = newTail;

        printf("\nserving from cached data\n");
    }

    // otherwise create a response by connecting to remote server
    // and cache the response
    else
    {
        printf("\nrequesting remote server for response\n");

        // create a response object + handle response errors
        int res_status = 1;
        if ((res_status = createResponse(cfd, response, request, responseBuffer, requestBuffer)) != 1)
        {
            handleSendingError(response, request->httpVersion, res_status, cfd);

            exitCleanUp(cfd, arg, clientArg->liveThreads, request, response, clientArg->mtx, clientArg->cond, requestBuffer, responseBuffer);

            return NULL;
        }

        // when cache becomes full remove the least used node
        if (isCacheFull(*(clientArg->head)))
            *(clientArg->tail) = removeCacheNode(*(clientArg->tail));

        // now cache the response for future use
        *(clientArg->head) = addCacheNode(*(clientArg->head), request->query_url, strlen(request->query_url), response->body, response->contentType);

        // if tail is null then point it to head
        if (*(clientArg->tail) == NULL)
        {
            printf("updating tail node\n");
            *(clientArg->tail) = *(clientArg->head);
        }
    }

    // now send the data back to client
    sendMessage(cfd, 0, "%s", responseBuffer);

    // free all the allocated resources
    exitCleanUp(cfd, arg, clientArg->liveThreads, request, response, clientArg->mtx, clientArg->cond, requestBuffer, responseBuffer);

    return NULL;
}

// send welcome message to client
int sendWelcomeMessage(int fd, HttpResponse *response, const char *httpVersion)
{
    char responseBuffer[RESPONSE_BUFFER_SIZE];
    ssize_t bytesSent = 0;
    char body[] = "<html><body><h1>Welcome to Proxy!</h1></body></html>";

    initHttpResponse(response);

    // create an http response
    response->statusCode = 200;
    strcpy(response->httpVersion, httpVersion);
    strcpy(response->statusMessage, "OK");
    response->body = strdup(body);
    strcpy(response->contentType, "text/html");
    response->contentLength = (int)strlen(response->body);
    response->isChunked = 0;

    // parse response into string
    int actualBufferSize = unparseHttpResponse(response, responseBuffer, RESPONSE_BUFFER_SIZE);

    // send the response to client
    bytesSent = sendMessage(fd, 0, "%s", responseBuffer);

    return bytesSent;
}

// send error message , returns the size of the message
int sendErrorMessage(int fd, HttpResponse *response, const char *httpVersion, int statusCode, const char *statusMessage, const char *body)
{
    char responseBuffer[RESPONSE_BUFFER_SIZE];
    ssize_t bytesSent = 0;

    // zero out the response object
    initHttpResponse(response);

    response->statusCode = statusCode;
    response->isChunked = 0;
    response->isRedirect = 0;
    response->body = strdup(body);
    response->contentLength = (int)strlen(response->body);
    strcpy(response->contentType, "text/plain");
    strcpy(response->httpVersion, httpVersion);
    strcpy(response->statusMessage, statusMessage);

    int actualBufferSize = unparseHttpResponse(response, responseBuffer, RESPONSE_BUFFER_SIZE);

    bytesSent = sendMessage(fd, 0, "%s", responseBuffer);

    return bytesSent;
}

void handleSendingError(HttpResponse *response, const char *http_version, int error_code, int cfd)
{
    switch (error_code)
    {
    case SERVCONNFAIL:
    {
        sendErrorMessage(cfd, response, http_version, 500, "Internal Server Error", "Failed to Establish Connection with Server");
        break;
    }
    case SERVREQFAIL:
    {
        sendErrorMessage(cfd, response, http_version, 500, "Internal Server Error", "Failed to Request to Remote Server");
        break;
    }
    case SERVRESFAIL:
    {
        sendErrorMessage(cfd, response, http_version, 500, "Internal Server Error", "Failed to Receive Data from Remote Server!\n");
        break;
    }
    case BADSERVRES:
    {
        sendErrorMessage(cfd, response, http_version, 500, "Internal Server Error", "Bad Response Received from Server");
        break;
    }
    case CLNTREQFAIL:
    {
        sendErrorMessage(cfd, response, http_version, 500, "Internal Server Error", "Failed to Receive Request From Client");
        break;
    }
    case BADCLNTREQ:
    {
        sendErrorMessage(cfd, response, http_version, 400, "Bad Request", "Invalid Request Received!");
        break;
    }
    case MISQRYPRM:
    {
        sendErrorMessage(cfd, response, http_version, 400, "Bad Request", "Query Url Must Be Present in this Case!");
        break;
    }
    }
}
