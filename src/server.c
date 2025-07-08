#include "../include/server.h"

void start_server(int port, const char *ip)
{
    struct sockaddr_in server_addr = {0};
    socklen_t addrLen = 0;

    if (!ip)
        ip = "127.0.0.1";

    // create a socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0);

    // initialize the address with 0
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, (struct in_addr *)&(server_addr.sin_addr)) <= 0)
    {
        perror("inet_pton");
        return;
    }

    int yes = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    addrLen = sizeof(struct sockaddr_in);
    if (bind(sfd, (struct sockaddr *)&server_addr, addrLen) < 0)
    {
        perror("bind");
        return;
    }

    if (listen(sfd, BACKLOG_SIZE) < 0)
    {
        perror("listen");
        return;
    }

    printf("server is listening on port %d...\n", port);

    ensure_cache_dir();
    CacheLRU *cache = init_cache_lru(MAX_CACHE_SIZE);

    // getting blocked sites
    char *blocked_sites[MAX_BLOCKED_SITES];
    int n_of_b_sites = get_blocked_sites(blocked_sites, MAX_BLOCKED_SITES);

    while (1)
    {
        struct sockaddr_in client_addr = {0};
        socklen_t addrlen = sizeof(client_addr);
        
        // print current cache
        print_cache_list(cache);

        // accept the upcoming client
        int client_fd = accept(sfd, (struct sockaddr *)&client_addr, &addrlen);

        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        // Right now, handle in main thread
        handle_client(client_fd, cache, blocked_sites, n_of_b_sites);

        // Later: spawn thread
        /*
        pthread_t tid;
        pthread_create(&tid, NULL, thread_fn, (void *)(intptr_t)client_fd);
        */
    }
}
