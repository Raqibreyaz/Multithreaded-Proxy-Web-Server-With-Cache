#include "utils/socket-library.h"

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

    int myFd = createServer(AF_INET, SOCK_STREAM, port, BACKLOG_SIZE, "127.0.0.1", (struct sockaddr_storage *)&myAddr);

    while (1)
    {
        int cfd;
        HttpRequest request;
        HttpResponse response;
        ssize_t receivedBytes = 0, sentBytes = 0;

        if ((cfd = acceptClient(myFd, NULL, NULL)) == -1)
        {
            printf("failed to connect to client\n");
            continue;
        }

        if ((receivedBytes = recvAllData(cfd, requestBuffer, REQUEST_BUFFER_SIZE, 0)) < 0)
        {
            sendErrorMessage(cfd, &response, request.httpVersion, 500, "Internal Server Error", "Failed to Receive Request from Client");
            close(cfd);
            continue;
        }

        int parseStatus = 0;
        if ((parseStatus = parseHttpRequest(requestBuffer, &request)) <= 0)
        {
            if (parseStatus != 0)
                sendErrorMessage(cfd, &response, request.httpVersion, 400, "Bad Request", "Invalid Request Received");
            close(cfd);
            continue;
        }

        // when our home page is requested then respond the home page
        if (strncmp(request.host, "localhost", 9) == 0 && strcmp(request.path, "/") == 0)
        {
            sendWelcomeMessage(cfd, &response, request.httpVersion);
        }

        // when a remote page is requested
        else if (strncmp(request.host, "localhost", 9) != 0)
        {
            // create raw request string from request object
            int requestBufferSize = unparseHttpRequest(&request, requestBuffer, REQUEST_BUFFER_SIZE);

            // create connection to the remote server
            int sfd = createConnection(AF_INET, SOCK_STREAM, request.host, "http", (struct sockaddr_storage *)&serverAddr);

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
            // now send the data back to client
            sendMessage(cfd, 0, "%s", responseBuffer);
        }

        close(cfd);
    }

    return 0;
}
