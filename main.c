#include "utils/socket-library.h"

#define BACKLOG_SIZE 10

int main(int argc, char const *argv[])
{
    int port;

    if (argc < 2 || (port = extractNumber(argv[1], 4)) == -1)
        exitWithMessage("please provide 4 digit port number\n");

    char requestBuffer[REQUEST_BUFFER_SIZE];
    char responseBuffer[RESPONSE_BUFFER_SIZE];
    struct sockaddr_in myAddr, serverAddr;

    int myFd = createServer(AF_INET, SOCK_STREAM, port, BACKLOG_SIZE, "127.0.0.1", (struct sockaddr_storage *)&myAddr);

    while (1)
    {
        HttpRequest request;
        HttpResponse response;
        ssize_t receivedBytes = 0;

        int cfd;
        if ((cfd = acceptClient(myFd, NULL, NULL)) == -1)
        {
            printf("failed to connect to client");
            continue;
        }

        printf("client connected %d\n", cfd);

        // receive all the data from client
        if ((receivedBytes = recvAllData(cfd, requestBuffer, REQUEST_BUFFER_SIZE, 0)) < 0)
            continue;

        printf("%d bytes data received from client\n", (int)receivedBytes);

        parseHttpRequest(requestBuffer, &request);

        if (strcmp(request.url, "/") == 0 && sendWelcomeMessage(cfd, &response) == -1)
            printf("failed to respond to client\n");

        // chekcing if we got a website url
        else if (strncmp(request.url, "/", 1) == 0 && strcmp(request.url, "/favicon.ico") != 0)
        {
            printf("requested url: %s\n", request.url + 1);
            // create a connection with that remote server
            int sfd = createConnection(AF_INET, SOCK_STREAM, request.url + 1, "http", (struct sockaddr_storage *)&serverAddr);

            sendMessage(sfd, 0, requestBuffer);

            // receive response from remote server
            receivedBytes = recvAllData(sfd, responseBuffer, RESPONSE_BUFFER_SIZE, 0);

            // directly send the response recieved from the remote server
            sendMessage(cfd, 0, "%s", responseBuffer);
            // send(cfd, responseBuffer, receivedBytes, 0);
        }

        // after sending the response free the allocated space
        freeHttpRequest(&request);
        close(cfd);
    }

    return 0;
}
