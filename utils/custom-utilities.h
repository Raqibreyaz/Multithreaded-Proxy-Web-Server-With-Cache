// - a function which terminate program by closing file-desc with print error message by errno
// - a function which terminate program and print error message given
// perror
// close

#ifndef CUSTOM_UTILITIES_H
#define CUSTOM_UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <arpa/inet.h>

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

#endif