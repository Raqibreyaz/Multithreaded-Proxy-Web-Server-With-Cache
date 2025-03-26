#include "socket-library/socket-library.h"
#include "cache/cache-list.h"

#define BACKLOG_SIZE 10

// forward request to remote server
// received response from server and send back to client
int main(int argc, char const *argv[])
{

    int port;

    if (argc < 2 || (port = extractNumber(argv[1], 4)) == -1)
        exitWithMessage("please provide valid 4 digit port number!\n");

    char requestBuffer[REQUEST_BUFFER_SIZE];
    char responseBuffer[RESPONSE_BUFFER_SIZE];
    struct sockaddr_in myAddr, serverAddr;
    CacheNode *head = NULL;

    int myFd = createServer(AF_INET, SOCK_STREAM, port, BACKLOG_SIZE, "127.0.0.1", (struct sockaddr_storage *)&myAddr);

    while (1)
    {
        int cfd;
        HttpRequest request;
        HttpResponse response;
        ssize_t receivedBytes = 0, sentBytes = 0;

        // accept client connection
        if ((cfd = acceptClient(myFd, NULL, NULL)) == -1)
        {
            printf("failed to connect to client\n");
            continue;
        }

        // receive entire request from client
        if ((receivedBytes = recvAllData(cfd, requestBuffer, REQUEST_BUFFER_SIZE, 0)) < 0)
        {
            sendErrorMessage(cfd, &response, request.httpVersion, 500, "Internal Server Error", "Failed to Receive Request from Client");
            close(cfd);
            continue;
        }

        // parse the http request
        int parseStatus = 0;
        if ((parseStatus = parseHttpRequest(&request, requestBuffer)) <= 0)
        {
            if (parseStatus != 0)
                sendErrorMessage(cfd, &response, request.httpVersion, 400, "Bad Request", "Invalid Request Received");
            close(cfd);
            continue;
        }

        // when our home page is requested then respond the home page
        if (strncmp(request.host, "localhost:", 10) == 0 && strcmp(request.path, "/") == 0)
        {
            printf("sending welcome message\n");
            sendWelcomeMessage(cfd, &response, request.httpVersion);
        }

        // when a remote page is requested then respond the remote page
        else if (strncmp(request.host, "localhost:", 10) != 0)
        {

            // find the cached response
            HttpResponse *res = findCacheNode(head, request.host, request.path);

            // if cached response found then use it
            if (res != NULL)
            {
                response = *res;
                printf("\nserving from cached data\n");
            }

            // if no cached response available then forward the request to original server
            else
            {
                printf("\nno cached data available\n");

                // create raw request string from request object
                int requestBufferSize = unparseHttpRequest(&request, requestBuffer, REQUEST_BUFFER_SIZE);

                // create connection to the remote server
                int sfd = createConnection(AF_INET, SOCK_STREAM, request.host, "http", (struct sockaddr_storage *)&serverAddr, 0);

                if (sfd == -1)
                {
                    sendErrorMessage(cfd, &response, request.httpVersion, 400, "Server Connection Failed", "Failed to Establish Connection with Server");
                    close(cfd);
                    continue;
                }

                // forward request to remote server
                if ((sentBytes = sendMessage(sfd, 0, "%s", requestBuffer)) == -1)
                {
                    sendErrorMessage(cfd, &response, request.httpVersion, 500, "Internal Server Error", "Failed to Request to Remote Server");
                    close(sfd);
                    close(cfd);
                    continue;
                }

                // receive data from remote server
                if ((receivedBytes = recvAllData(sfd, responseBuffer, RESPONSE_BUFFER_SIZE, 0)) == -1)
                {
                    sendErrorMessage(cfd, &response, request.httpVersion, 500, "Internal Server Error", "Failed to Receive Data from Remote Server!\n");
                    close(sfd);
                    close(cfd);
                    continue;
                }

                // close connection from the server
                close(sfd);

                int parseStatus;
                if ((parseStatus = parseHttpResponse(&response, responseBuffer, request.host, request.path)) <= 0)
                {
                    if (parseStatus != 0)
                        sendErrorMessage(cfd, &response, request.httpVersion, 500, "Bad Response", "Bad Response Received from Server");
                    close(cfd);
                    continue;
                }

                // now cache the response for future use
                head = addCacheNode(head, &response);
            }

            // create the raw response string
            unparseHttpResponse(&response, responseBuffer, RESPONSE_BUFFER_SIZE);

            // now send the data back to client
            sendMessage(cfd, 0, "%s", responseBuffer);
        }

        // close connection from the client
        close(cfd);

        // freeing the allocated space
        freeHttpResponse(&response);
    }
    return 0;
}
