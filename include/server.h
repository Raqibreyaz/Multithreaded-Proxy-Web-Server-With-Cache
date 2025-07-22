#ifndef SERVER_H
#define SERVER_H
#include "client-handler.h"
#include "blocked-sites.h"
#include "client-queue.h"
#include "thread-pool.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>

#define BACKLOG_SIZE 10
#define MAX_CLIENTS 10
#define MAX_BLOCKED_SITES 100
#define MAX_CACHE_SIZE 100

void server_shutdown_handler(int sig);

int create_server(int port, const char *ip);

void start_server(int port, const char *ip);

#endif