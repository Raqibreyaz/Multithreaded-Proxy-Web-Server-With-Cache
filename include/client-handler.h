#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H
#define MAX_REDIRECTS_ALLOWED 4

#include "http-request-response.h"
#include "http-parser.h"
#include "blocked-sites.h"
#include "utils.h"
#include <unistd.h>

void handle_client(int client_fd, CacheLRU *cache, char *blocked_sites[], int n_of_b_sites);

#endif