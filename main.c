#include "include/server.h"

#define PORT 4040

// forward request to remote server
// received response from server and send back to client
int main(int argc, char const *argv[])
{
    int port = PORT;

    // extracting port from env vars
    const char *port_string = getenv("PORT");
    if (port_string)
        port = atoi(port_string);

    // starting proxy server on wilcard address
    start_server(port, "0.0.0.0");

    return 0;
}
