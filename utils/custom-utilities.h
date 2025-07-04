// - a function which terminate program by closing file-desc with print error message by errno
// - a function which terminate program and print error message given
// perror
// close

#ifndef CUSTOM_UTILITIES_H
#define CUSTOM_UTILITIES_H

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#define BLOCKED_SITES_FILE "blocked-sites.json"
#define MAX_AD_PATTERNS 3

// will print error and close the file descriptor and exit
void fatalWithClose(int fd, const char *msg);

// will print error and exit
void fatal(const char *msg);

// will print message and exit
void exitWithMessage(const char *__restrict__ msg);

// will print message and close file descriptor and exit
void exitAndCloseWithMessage(int fd, const char *__restrict__ msg);

// will convert the string ip to binary
void convertToBinaryIP(int fd, int domain, const char *ip, void *addr);

// will convert the binary ip to string
const char *convertBinaryIPToString(int fd, int domain, void *addr, char *buffer, socklen_t bufferSize);

int extractNumber(const char *num, size_t len);

char *convertNumberToString(size_t num);

int isIpAddress(int domain, const char *str);

char *trim_whitespace(char *str);

void remove_quotes(char *str);

void unescape_string(char *str);

// will fill the array with blocked sites and returns the no of blocked sites
int get_blocked_sites(char *blocked_sites[], int max_items);

// will check if the site is blocked using it's url
int is_site_blocked(const char *blocked_sites[], int n, const char *url);

// will return the no of minutes passed after the last modification of file
long time_passed_in_mins_for_file(const char *file_path);

// will read and return contents of that file, file must be present
char *read_file(const char *file_path);

// will write the contents to the file, file will be created if not present
void write_file(const char *file_path, const char *data);

// delete the provided file
void delete_file(const char *file_path);

// get the query url from given path
char *get_url_path_from_query(const char *query);

// get the host from url
char *get_host_from_query(const char *query);

// will replace all the problematic chars from filename
void sanitize_filename(char *file_name);

#endif