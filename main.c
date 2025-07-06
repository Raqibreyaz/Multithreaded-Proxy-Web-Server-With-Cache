#include "socket-library/socket-library.h"
#include "cache-list/cache-list.h"
#include <pthread.h>

#define BACKLOG_SIZE 10
#define MAX_CLIENTS 10
#define MAX_BLOCKED_SITES 100
#define PORT 3000

void cleanup_func(int cfd, HttpRequest *request, HttpResponse *response, char *request_buffer, char *response_buffer)
{
    // closing connection to client
    if (cfd != -1)
        close(cfd);

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
    if (request_buffer)
        free(request_buffer);

    // freeing response buffer
    if (response_buffer)
        free(response_buffer);
}

void handle_non_thread_client(struct ClientHandlerArg clientArg)
{
    int cfd = clientArg.cfd;

    HttpRequest *request = (HttpRequest *)malloc(sizeof(HttpRequest));
    HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));
    char *requestBuffer = (char *)malloc(REQUEST_BUFFER_SIZE);
    char *responseBuffer = (char *)malloc(RESPONSE_BUFFER_SIZE);

    // if memory allocating failed then close connection and go back
    if (!request || !response || !requestBuffer || !responseBuffer)
    {
        close(cfd);
        return;
    }

    // accept request from client + handle requesting error
    int req_status = 1;
    if ((req_status = acceptRequest(cfd, request, requestBuffer)) != 1)
    {
        handleSendingError(response, "1.1", req_status, cfd);

        cleanup_func(cfd, request, response, requestBuffer, responseBuffer);

        return;
    }

    // when our home page is requested then respond the home page
    if (strncmp(request->host, "localhost:", 10) == 0 && strcmp(request->path, "/") == 0)
    {
        printf("sending welcome message\n");

        sendWelcomeMessage(cfd, response, request->httpVersion);

        // freeing all the allocated resources
        cleanup_func(cfd, request, response, requestBuffer, responseBuffer);

        return;
    }

    // handle if no query url present and no favicon icon requested
    if (strlen(request->query_url) == 0 && strcmp(request->path, "/favicon.ico"))
    {
        handleSendingError(response, request->httpVersion, MISQRYPRM, cfd);

        cleanup_func(cfd, request, response, requestBuffer, responseBuffer);

        return;
    }

    const char *url = strcmp(request->path, "/favicon.ico") == 0 ? request->path + 1 : request->query_url;

    // check if there is a cached response available
    CacheNode *res = findCacheNode(*(clientArg.head), url);

    // if cached response found then use it
    if (res != NULL)
    {
        // prepare the full file path
        char file_path[1050];
        int bytes_written = snprintf(file_path, sizeof(file_path) - sizeof(".meta") - 1, "%s/%s", CACHE_DIR, res->url);
        file_path[bytes_written] = '\0';

        // read the cache file
        size_t data_file_size = 0;
        char *data = read_file(file_path, &data_file_size);

        // prepare the data type file path
        bytes_written += snprintf(file_path + bytes_written, sizeof(file_path) - bytes_written - 1, ".meta");
        file_path[bytes_written] = '\0';

        // read the data type file
        size_t data_type_file_size = 0;
        char *data_type = read_file(file_path, &data_type_file_size);

        response->statusCode = 200;
        strcpy(response->statusMessage, "Successful");
        strcpy(response->httpVersion, request->httpVersion);
        strcpy(response->contentType, data_type);
        response->contentLength = data_file_size;
        response->isChunked = 0;
        response->isRedirect = 0;
        response->body = data;

        // freeing the allocated space for the data type
        free(data_type);

        // move the node to head as it is recently used
        CacheNode *newTail = moveToHead(*(clientArg.head), res);

        // update the new head
        *(clientArg.head) = res;

        // update the tail if modified
        if (newTail)
            *(clientArg.tail) = newTail;

        printf("\nserving from cached data\n");
    }

    // otherwise create a response by connecting to remote server
    // and cache the response
    else
    {
        printf("\nrequesting remote server for response\n");

        // create a response object + handle response errors
        int res_status = 1;
        if ((res_status = createResponse(cfd, response, request, responseBuffer, requestBuffer)) != 0)
        {
            handleSendingError(response, request->httpVersion, res_status, cfd);
            cleanup_func(cfd, request, response, requestBuffer, responseBuffer);

            return;
        }

        // when cache becomes full remove the least used node
        if (isCacheFull(*(clientArg.head)))
            *(clientArg.tail) = removeCacheNode(*(clientArg.tail));

        // rewrite the html for urls
        char url[URL_SIZE];
        snprintf(url, URL_SIZE, "https://%s/%s", request->host, request->path);
        char *new_body = rewrite_html_to_proxy(response->body, url);

        free(response->body);
        response->body = new_body;
        new_body = NULL;

        // now cache the response for future use
        *(clientArg.head) = addCacheNode(*(clientArg.head),
                                         request->query_url,
                                         strlen(request->query_url),
                                         response->body,
                                         response->contentLength,
                                         response->contentType,
                                         strlen(response->contentType));

        // if tail is null then point it to head
        if (*(clientArg.tail) == NULL)
        {
            printf("updating tail node\n");
            *(clientArg.tail) = *(clientArg.head);
        }
    }

    unparseHttpResponse(response, responseBuffer, RESPONSE_BUFFER_SIZE);

    // now send the data back to client
    sendMessage(cfd, 0, "%s", responseBuffer);

    // free all the allocated resources
    cleanup_func(cfd, request, response, requestBuffer, responseBuffer);

    return;
}

// forward request to remote server
// received response from server and send back to client
int main(int argc, char const *argv[])
{
    // int liveThreads = 0;

    // initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // creating static mutex and cond vars
    // pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    // pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    struct sockaddr_in myAddr;

    // get the cache list
    ListPair list_pair = populateCacheWithDisk();

    // get the blocked sites
    char *blocked_sites[MAX_BLOCKED_SITES];
    int no_of_blocked_sites = get_blocked_sites(blocked_sites, MAX_BLOCKED_SITES);

    // create a server at loopback address
    int myFd = createServer(AF_INET, SOCK_STREAM, PORT, BACKLOG_SIZE, "127.0.0.1", (struct sockaddr_storage *)&myAddr);

    while (1)
    {
        int cfd = acceptClient(myFd, NULL, NULL);
        if (cfd == -1)
        {
            perror("accept");
            continue;
        }

        struct ClientHandlerArg arg = {.cfd = cfd,
                                       .cond = NULL,
                                       .head = &(list_pair.head),
                                       .tail = &(list_pair.tail)};

        handle_non_thread_client(arg);
    }

    // while (1)
    // {
    // int cfd;

    // // wait for clients to finish to get free space
    // pthread_mutex_lock(&mtx);
    // while (liveThreads >= MAX_CLIENTS)
    // {
    //     printf("main thread is waiting for signal\n");
    //     pthread_cond_wait(&cond, &mtx);
    // }
    // pthread_mutex_unlock(&mtx);

    // // accept client connection
    // if ((cfd = acceptClient(myFd, NULL, NULL)) == -1)
    // {
    //     printf("failed to connect to client\n");
    //     continue;
    // }

    // printf("client connected\n");

    // struct ClientHandlerArg *arg =
    //     (struct ClientHandlerArg *)malloc(sizeof(struct ClientHandlerArg));

    // arg->cfd = cfd;
    // arg->head = &(list_pair.head);
    // arg->tail = &(list_pair.tail);
    // arg->cond = &cond;
    // arg->mtx = &mtx;
    // arg->liveThreads = &liveThreads;

    // pthread_t thread;

    // // new thread is created
    // liveThreads++;

    // // assign the thread to serve this client
    // if (pthread_create(&thread, NULL, handleClient, arg) != 0)
    // {
    //     perror("pthread_create");
    //     close(cfd);
    //     free(arg);
    //     liveThreads--;
    //     continue;
    // }

    // printf("no of running threads: %d\n", liveThreads);

    // // we dont want any return value from thread
    // pthread_detach(thread);
    // }
    return 0;
}
