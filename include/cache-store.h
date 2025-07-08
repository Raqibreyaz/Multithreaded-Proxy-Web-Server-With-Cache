#ifndef CACHE_STORE_H
#define CACHE_STORE_H
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#define CACHE_DIR "cached"

void ensure_cache_dir();
char *get_cache_filename(const char *url);
int write_cache_file(const char *filename, const char *content_type, const char *data, size_t data_len);
char *read_cache_file(const char *filename, char *content_type_out, size_t *data_len_out);

#endif