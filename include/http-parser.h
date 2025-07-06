#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "fetch.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

HttpResponse* parse_http_response(const char *raw, size_t raw_len);

#endif