// - a function which terminate program by closing file-desc with print error message by errno
// - a function which terminate program and print error message given
// perror
// close
#include "custom-utilities.h"

void fatalWithClose(int fd, const char *msg)
{
    perror(msg);
    close(fd);
    exit(EXIT_FAILURE);
}
void fatal(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
void exitWithMessage(const char *__restrict__ msg)
{
    printf("%s\n",msg);
    exit(EXIT_FAILURE);
}

void exitAndCloseWithMessage(int fd, const char *__restrict__ msg)
{
    printf("%s\n",msg);
    close(fd);
    exit(EXIT_FAILURE);
}

void convertToBinaryIP(int fd, int domain, const char *ip, void *addr)
{
    if (inet_pton(domain, ip, addr) <= 0)
        fatalWithClose(fd, "inet_pton");
}

const char *convertBinaryIPToString(int fd, int domain, void *addr, char *buffer, socklen_t bufferSize)
{
    const char *ip = inet_ntop(domain, addr, buffer, bufferSize);
    if (ip == NULL)
        fatalWithClose(fd, "inet_ntop");
    return ip;
}

int extractNumber(const char *num, size_t len)
{

    int extracted_num = 0;
    int place_value = 1;

    for (int i = len - 1; i >= 0; i--)
    {
        if (num[i] < '0' || num[i] > '9')
            return -1;

        int digit = (int)(num[i] - '0');
        extracted_num = (int)(digit * place_value + extracted_num);
        place_value *= 10;
    }
    return extracted_num;
}

char *convertNumberToString(size_t num)
{

    char *strnum = (char *)malloc(20);

    // taking num as string
    sprintf(strnum, "%zu", num);

    return strnum;
}

// check if a string is an ip or not
int isIpAddress(int domain, const char *str)
{
    struct sockaddr_storage addr;

    // for ipv4 , if !ip then inetpton will return 0
    if (domain == AF_INET)
    {
        return !inet_pton(domain, str, (struct sockaddr_in *)&addr) == 0;
    }

    return !inet_pton(domain, str, (struct sockaddr_in6 *)&addr) == 0;
}