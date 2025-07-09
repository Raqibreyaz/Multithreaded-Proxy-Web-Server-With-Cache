#ifndef BLOCKED_SITES_H
#define BLOCKED_SITES_H
#define BLOCKED_SITES_FILE "blocked-sites.json"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"

// will fill the array with blocked sites and returns the no of blocked sites
int get_blocked_sites(char *blocked_sites[], int max_items);

// will check if the site is blocked using it's url
int is_site_blocked(char *blocked_sites[], int n, const char *url);

char *trim_whitespace(char *str);

void remove_quotes(char *str);

void unescape_string(char *str);

#endif