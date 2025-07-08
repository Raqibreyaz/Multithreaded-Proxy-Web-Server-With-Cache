#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char *read_file(const char *file_path, size_t *file_size);

void write_file(const char *file_path, const char *data, size_t data_size);

long time_passed_in_hours_for_file(const char *file_path);

int urls_are_equivalent(const char *a, const char *b);
#endif