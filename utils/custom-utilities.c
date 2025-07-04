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
    printf("%s\n", msg);
    exit(EXIT_FAILURE);
}

void exitAndCloseWithMessage(int fd, const char *__restrict__ msg)
{
    printf("%s\n", msg);
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
    if (!num || num[0] == '\0')
        return -1;

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

char *trim_whitespace(char *str)
{
    // skipping leading whitespace
    while (isspace(*str))
        str++;

    // skipping trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        *end-- = '\0';
    return str;
}

void remove_quotes(char *str)
{
    int len = strlen(str);
    if (str[0] == '"')
        memmove(str, str + 1, len--);
    if (str[len - 1] == '"')
        str[len - 1] = '\0';
}

void unescape_string(char *str)
{
    char *src = str, *dest = str;

    while (src && *src)
    {
        if (*src == "\\")
        {
            src++;
            switch (*src)
            {
            case 'n':
                *dest++ = '\n';
                break;
            case 't':
                *dest++ = '\t';
                break;

            default:
                *dest++ = *src;
            }
        }
        else
        {
            *dest++ = *src;
        }
        src++;
    }
    *dest = '\0';
}

// will fill the array with blocked sites and returns the no of blocked sites
int get_blocked_sites(char *blocked_sites[], int max_items)
{
    const char *blocked_sites_string = read_file(BLOCKED_SITES_FILE);

    // skipping the '[' and ']'
    char *start = strchr(blocked_sites_string, '[');
    char *end = strchr(blocked_sites_string, ']');
    if (!start || !end || start >= end)
    {
        free(blocked_sites_string);
        return 0;
    }
    *end = '\0';
    start++;

    char *token = strtok(start, ",");
    int count = 0;

    while (token && count < max_items)
    {
        // trim spaces
        token = trim_whitespace(token);

        // removed ""
        remove_quotes(token);

        // like \n is in 2 bytes here , storing in 1byte '\n'
        unescape_string(token);

        // allocating copy in heap
        blocked_sites[count++] = strdup(token);

        // moving to next token
        token = strtok(NULL, "");
    }

    free(blocked_sites_string);

    return count;
}

// will check if the site is blocked using it's url
int is_site_blocked(const char *blocked_sites[], int no_of_blocked_sites, const char *url)
{
    for (int i = 0; i < no_of_blocked_sites; i++)
    {
        if (strcmp(blocked_sites[i], url) == 0)
            return 1;
    }
    return 0;
}

// will return the no of minutes passed after the last modification of file
long time_passed_in_mins_for_file(const char *file_path)
{
    // taking stats of file
    struct stat st;
    stat(file_path, &st);

    // getting the current time
    time_t current_time = time(NULL);

    // returning the time passed in minutes
    return ceil(difftime(current_time, st.st_mtime) / 60);
}

// will read and return contents of that file, file must be present
char *read_file(const char *file_path)
{
    // getting stats of the file
    struct stat st;
    stat(file_path, &st);

    // allocating the file size bytes
    char *data = (char *)malloc(st.st_size + 1);

    // opening file in binary mode
    FILE *fptr = fopen(file_path, "rb");
    if (!fptr)
    {
        printf("failed to read file: %s", file_path);
        return NULL;
    }

    // get the exact size bytes from file and \0 terminating
    fgets(data, st.st_size, fptr);
    data[st.st_size] = '\0';

    fclose(fptr);

    // returning the data
    return data;
}

// will write the contents to the file, file will be created if not present
void write_file(const char *file_path, const char *data)
{
    // opening the file in write + binary mode
    FILE *fptr = fopen(file_path, "wb");
    if (!fptr)
    {
        printf("failed to write file: %s\n", file_path);
        return NULL;
    }

    // now write the data to the file
    fprintf(fptr, "%s", data);
    fclose(fptr);
}

// delete the given file
void delete_file(const char *file_path)
{
    if (remove(file_path) != 0)
        printf("failed to delete the file: %s\n", file_path);
}

// get the query url from given path
char *get_url_path_from_query(const char *url)
{
    // if http:// then skip it
    if (strncmp(url, "http://", 7) == 0)
        url += 7;
    // if https:// then skip it too!
    else if (strncmp(url, "https://", 8) == 0)
        url += 8;

    char *slash = strchr(url, '/');
    if (slash)
    {
        int path_len = strlen(slash);
        char *path = (char *)malloc(path_len);
        strcpy(path, slash + 1);
        path[path_len - 1] = '\0';
    }
    return NULL;
}

// get the host from url
char *get_host_from_query(const char *url)
{
    // if http:// then skip it
    if (strncmp(url, "http://", 7) == 0)
        url += 7;
    // if https:// then skip it too!
    else if (strncmp(url, "https://", 8) == 0)
        url += 8;

    char *slash = strchr(url, '/');

    if (slash)
    {
        int len = slash - url;
        char *host = (char *)malloc(len + 1);
        strncpy(host, url, len);
        host[len] = '\0';
        return host;
    }

    return strdup(url);
}

// will replace all the problematic chars from filename
void sanitize_filename(char *file_name)
{
    const char *invalid = "<>:\"/\\|?*";
    for (int i = 0; file_name[i]; i++)
    {
        unsigned char ch = file_name[i];
        if (ch < 32 || strchr(invalid, ch))
            file_name[i] = '-';
    }
}

const char const *ad_patterns[MAX_AD_PATTERNS] = {

};

void remove_ad_matches(char *html, const char *pattern)
{
}

void remove_ad_tags(char *html)
{
}