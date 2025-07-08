#include "../include/blocked-sites.h"

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
int is_site_blocked(char *blocked_sites[], int no_of_blocked_sites, const char *url)
{
    for (int i = 0; i < no_of_blocked_sites; i++)
    {
        char *blocked_site = blocked_sites[i];
        if (strstr(url, blocked_site))
            return 1;
    }
    return 0;
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
