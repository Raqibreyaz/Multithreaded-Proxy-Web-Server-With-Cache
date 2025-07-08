#include "include/server.h"

#define PORT 3000

// forward request to remote server
// received response from server and send back to client
int main(int argc, char const *argv[])
{
    start_server(PORT, "0.0.0.0");

    return 0;
}
