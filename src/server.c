#include "../include/server.h"

void server_shutdown_handler(int sig)
{
    printf("gracefull shutting down server...\n");
    exit(EXIT_SUCCESS);
}

int create_server(int port, const char *ip)
{
    if (port <= 0 || !ip || !*ip)
        return -1;

    struct sockaddr_in server_addr = {0};
    socklen_t addrLen = 0;

    // create a socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
    {
        perror("socket");
        return -1;
    }

    // initialize the address with 0
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, (struct in_addr *)&(server_addr.sin_addr)) <= 0)
    {
        perror("inet_pton");
        close(sfd);
        return -1;
    }

    int yes = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    addrLen = sizeof(struct sockaddr_in);
    if (bind(sfd, (struct sockaddr *)&server_addr, addrLen) < 0)
    {
        perror("bind");
        close(sfd);
        return -1;
    }

    if (listen(sfd, BACKLOG_SIZE) < 0)
    {
        perror("listen");
        close(sfd);
        return -1;
    }

    printf("server is listening on port %d...\n", port);

    return sfd;
}

void start_server(int port, const char *ip)
{
    if (port <= 0)
        port = 8080;

    if (!ip || !*ip)
        ip = "127.0.0.1";

    // create a server on that port and ipu
    int sfd = create_server(port, ip);
    if (sfd < 0)
        exit(EXIT_FAILURE);

    // ensuring cache directory exist
    ensure_cache_dir();

    // create the cache list from cache dir
    CacheLRU *cache = init_cache_lru(MAX_CACHE_SIZE);
    if (!cache)
        exit(EXIT_FAILURE);

    // getting blocked sites
    char *blocked_sites[MAX_BLOCKED_SITES];
    int n_of_b_sites = get_blocked_sites(blocked_sites, MAX_BLOCKED_SITES);

    // create the client queue
    ClientQueue client_queue;
    init_client_queue(&client_queue);

    // create the cache lock var
    pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;

    // a context which will be shared across threads
    SharedContext shared_ctx = {.blocked_sites = blocked_sites,
                                .cache = cache,
                                .cache_lock = &cache_lock,
                                .client_queue = &client_queue,
                                .n_of_b_sites = n_of_b_sites};

    // initialize the thread pool
    init_thread_pool(&shared_ctx);

    // ignore server crash if client disconnects in between
    signal(SIGPIPE, SIG_IGN);

    // gracefully shutdown the server on ctrl+c
    signal(SIGINT, server_shutdown_handler);

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

        // add the client to queue
        enqueue_client(&client_queue, client_fd);
    }
}
