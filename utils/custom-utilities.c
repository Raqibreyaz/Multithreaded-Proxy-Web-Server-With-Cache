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
        if (*src == '\\')
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
    size_t blocked_sites_file_size = 0;
    char *blocked_sites_string = read_file(BLOCKED_SITES_FILE, &blocked_sites_file_size);

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
        token = strtok(NULL, ",");
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
    return difftime(current_time, st.st_mtime) / 60;
}

// will read and return contents of that file, file must be present
char *read_file(const char *file_path, size_t *file_size)
{
    // opening file in binary mode
    FILE *fptr = fopen(file_path, "rb");
    if (!fptr)
    {
        printf("failed to read file: %s", file_path);
        return NULL;
    }

    // getting stats of the file
    struct stat st;
    stat(file_path, &st);

    // allocating the file size bytes
    char *data = (char *)malloc(st.st_size + 1);
    if (!data)
    {
        printf("memory allocation failed!\n");
        fclose(fptr);
        return NULL;
    }

    // get the exact size bytes from file and \0 terminating
    size_t bytes_read = fread(data, 1, st.st_size, fptr);
    data[st.st_size] = '\0';

    if (bytes_read < (size_t)st.st_size)
    {

        if (feof(fptr))
            printf("EOF reached, data not read!\n");
        if (ferror(fptr))
            perror("fread error");
    }

    // storing the file size in the provided var
    if (file_size)
        *file_size = (size_t)st.st_size;

    fclose(fptr);

    // returning the data
    return data;
}

// will write the contents to the file, file will be created if not present
void write_file(const char *file_path, const char *data, size_t data_size)
{
    // opening the file in write + binary mode
    FILE *fptr = fopen(file_path, "wb");
    if (!fptr)
    {
        printf("failed to write file: %s\n", file_path);
        return;
    }

    // now write the data to the file
    fwrite(data, 1, data_size, fptr);
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

char *insert_base_tag(const char *html, const char *domain) {
    const char *base_fmt = "<base href=\"/?url=%s/\">";
    char base_tag[1024];
    snprintf(base_tag, sizeof(base_tag), base_fmt, domain);

    const char *head_pos = strcasestr(html, "<head>");
    if (!head_pos) return strdup(html); // no <head>

    size_t pre_len = head_pos - html + strlen("<head>");
    size_t html_len = strlen(html);

    char *result = malloc(html_len + strlen(base_tag) + 1);
    if (!result) return NULL;
    strncpy(result, html, pre_len);
    result[pre_len] = '\0';
    strcat(result, base_tag);
    strcat(result, html + pre_len);

    return result;
}

// Rewrite absolute URLs like href="https://..."
char *rewrite_absolute_urls(const char *html) {
    regex_t regex;
    const char *pattern = "\\b(action|src|href|content|data|poster)=[\"'](https?://[^\"']+)[\"']";
    if (regcomp(&regex, pattern, REG_EXTENDED | REG_ICASE) != 0) return strdup(html);

    regmatch_t matches[3];
    const char *cursor = html;
    char *result = calloc(1, MAX_RESULT_LEN);
    if (!result) return NULL;
    size_t written = 0;

    while (cursor && *cursor && regexec(&regex, cursor, 3, matches, 0) == 0) {
        if (matches[0].rm_so < 0) break;

        strncat(result + written, cursor, matches[0].rm_so);
        written += matches[0].rm_so;

        int attr_len = matches[1].rm_eo - matches[1].rm_so;
        int url_len = matches[2].rm_eo - matches[2].rm_so;
        if (attr_len <= 0 || url_len <= 0 || attr_len >= MAX_ATTR_LEN || url_len >= MAX_URL_LEN) break;

        char attr[MAX_ATTR_LEN], url[MAX_URL_LEN];
        strncpy(attr, cursor + matches[1].rm_so, attr_len);
        attr[attr_len] = '\0';
        strncpy(url, cursor + matches[2].rm_so, url_len);
        url[url_len] = '\0';

        written += snprintf(result + written, MAX_RESULT_LEN - written, "%s=\"/?url=%s\"", attr, url);
        cursor += matches[0].rm_eo;
    }

    if (cursor) strncat(result + written, cursor, MAX_RESULT_LEN - written - 1);
    regfree(&regex);
    return result;
}

// Rewrite relative or protocol-relative URLs
char *rewrite_relative_urls(const char *html, const char *domain) {
    regex_t regex;
    const char *pattern = "\\b(action|src|href|content|data|poster)=[\"'](?!https?:|//|data:)([^\"']+)[\"']";
    if (regcomp(&regex, pattern, REG_EXTENDED | REG_ICASE) != 0) return strdup(html);

    regmatch_t matches[3];
    const char *cursor = html;
    char *result = calloc(1, MAX_RESULT_LEN);
    if (!result) return NULL;
    size_t written = 0;

    while (cursor && *cursor && regexec(&regex, cursor, 3, matches, 0) == 0) {
        if (matches[0].rm_so < 0) break;

        strncat(result + written, cursor, matches[0].rm_so);
        written += matches[0].rm_so;

        int attr_len = matches[1].rm_eo - matches[1].rm_so;
        int path_len = matches[2].rm_eo - matches[2].rm_so;
        if (attr_len <= 0 || path_len <= 0 || attr_len >= MAX_ATTR_LEN || path_len >= MAX_URL_LEN) break;

        char attr[MAX_ATTR_LEN], path[MAX_URL_LEN];
        strncpy(attr, cursor + matches[1].rm_so, attr_len);
        attr[attr_len] = '\0';
        strncpy(path, cursor + matches[2].rm_so, path_len);
        path[path_len] = '\0';

        char normalized[MAX_URL_LEN];
        snprintf(normalized, sizeof(normalized), "%s%s", path[0] == '/' ? "" : "/", path);

        written += snprintf(result + written, MAX_RESULT_LEN - written, "%s=\"/?url=%s%s\"", attr, domain, normalized);
        cursor += matches[0].rm_eo;
    }

    if (cursor) strncat(result + written, cursor, MAX_RESULT_LEN - written - 1);
    regfree(&regex);
    return result;
}

// Full pipeline to rewrite an HTML document
char *rewrite_html_to_proxy(const char *html, const char *domain) {
    if (!html || !domain) return NULL;
    char *step1 = insert_base_tag(html, domain);
    if (!step1) return NULL;
    char *step2 = rewrite_absolute_urls(step1);
    free(step1);
    if (!step2) return NULL;
    char *step3 = rewrite_relative_urls(step2, domain);
    free(step2);
    return step3;
}