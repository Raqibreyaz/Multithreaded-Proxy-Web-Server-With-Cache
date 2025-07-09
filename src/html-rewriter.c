#include "../include/html-rewriter.h"

static const char *ATTRIBUTES[] = {"href", "src", "action", "poster", "data", "content"};
static const int ATTR_COUNT = 6;

void normalize_domain(const char *input, char *output)
{
    if (!input || !output)
        return;

    if (strncmp(input, "http://", 7) == 0 || strncmp(input, "https://", 8) == 0)
        strcpy(output, input);
    else
        sprintf(output, "https://%s", input);
}

char *inject_base_tag(const char *html, size_t html_len, const char *host)
{
    if (!html || !host)
        return NULL;

    const char *base_fmt = "<base href=\"/?url=%s/\">";
    char normalized_host[512];
    normalize_domain(host, normalized_host);

    char base_tag[512];
    snprintf(base_tag, sizeof(base_tag), base_fmt, normalized_host);

    const char *head_pos = strcasestr(html, "<head>");
    if (!head_pos)
        return strndup(html, html_len);

    size_t pre_len = head_pos - html + strlen("<head>");
    size_t base_len = strlen(base_tag);
    size_t new_len = html_len + base_len;

    char *result = malloc(new_len + 1);
    if (!result)
        return NULL;

    memcpy(result, html, pre_len);
    memcpy(result + pre_len, base_tag, base_len);
    memcpy(result + pre_len + base_len, html + pre_len, html_len - pre_len);
    result[new_len] = '\0';

    return result;
}

char *rewrite_attr(const char *attr, const char *value, const char *domain)
{
    if (!attr || !value || !domain)
        return NULL;

    char *result = NULL;

    if (strncmp(value, "http://", 7) == 0 || strncmp(value, "https://", 8) == 0)
    {
        result = malloc(strlen(attr) + strlen(value) + 20);
        sprintf(result, "%s=\"/?url=%s\"", attr, value);
    }
    else if (strncmp(value, "//", 2) == 0)
    {
        result = malloc(strlen(attr) + strlen(value) + 24);
        sprintf(result, "%s=\"/?url=https://%s\"", attr, value + 2);
    }
    else
    {
        const char *slash = (value[0] == '/') ? "" : "/";
        result = malloc(strlen(attr) + strlen(domain) + strlen(value) + 24);
        sprintf(result, "%s=\"/?url=%s%s%s\"", attr, domain, slash, value);
    }

    return result;
}

char *rewrite_links_inplace(const char *html, const char *host)
{
    if (!html || !host)
        return NULL;

    size_t len = strlen(html);
    char *rewritten = malloc(len * 4); // safer
    if (!rewritten)
        return NULL;

    size_t i = 0, j = 0;
    int in_script = 0, in_style = 0;

    while (html[i])
    {
        // Check <script> and </script>
        if (strncasecmp(&html[i], "<script", 7) == 0)
            in_script = 1;
        else if (strncasecmp(&html[i], "</script>", 9) == 0)
            in_script = 0;

        // Check <style> and </style>
        if (strncasecmp(&html[i], "<style", 6) == 0)
            in_style = 1;
        else if (strncasecmp(&html[i], "</style>", 8) == 0)
            in_style = 0;

        if (!in_script && !in_style)
        {
            int matched = 0;

            for (int a = 0; a < ATTR_COUNT; a++)
            {
                size_t attr_len = strlen(ATTRIBUTES[a]);

                if (strncasecmp(&html[i], ATTRIBUTES[a], attr_len) == 0 &&
                    html[i + attr_len] == '=' &&
                    (html[i + attr_len + 1] == '"' || html[i + attr_len + 1] == '\''))
                {
                    char quote = html[i + attr_len + 1];
                    size_t val_start = i + attr_len + 2;
                    size_t val_end = val_start;

                    while (html[val_end] && html[val_end] != quote)
                        val_end++;

                    size_t value_len = val_end - val_start;
                    char *value = malloc(value_len + 1);
                    if (!value)
                        return NULL;
                    memcpy(value, &html[val_start], value_len);
                    value[value_len] = '\0';

                    char *new_attr = rewrite_attr(ATTRIBUTES[a], value, host);
                    free(value);

                    if (new_attr)
                    {
                        size_t new_len = strlen(new_attr);
                        memcpy(&rewritten[j], new_attr, new_len);
                        j += new_len;
                        free(new_attr);
                    }

                    i = val_end + 1;
                    matched = 1;
                    break;
                }
            }

            if (matched)
                continue;
        }

        rewritten[j++] = html[i++];
    }

    rewritten[j] = '\0';
    return rewritten;
}

char *rewrite_srcset(const char *srcset, const char *host)
{
    if (!srcset || !host)
        return NULL;

    char *copy = strdup(srcset);
    if (!copy)
        return NULL;

    char *result = malloc(strlen(srcset) * 3);
    if (!result)
    {
        free(copy);
        return NULL;
    }
    result[0] = '\0';

    char *token, *saveptr;
    token = strtok_r(copy, ",", &saveptr);
    while (token)
    {
        while (isspace(*token))
            token++;

        char url[1024] = {0}, size[64] = {0};
        if (sscanf(token, "%1023s %63s", url, size) >= 1)
        {
            char *rewritten = rewrite_attr("src", url, host);
            if (rewritten)
            {
                strcat(result, rewritten + 5); // skip src="
                if (strlen(size))
                {
                    strcat(result, " ");
                    strcat(result, size);
                }
                strcat(result, ", ");
                free(rewritten);
            }
        }

        token = strtok_r(NULL, ",", &saveptr);
    }

    size_t len = strlen(result);
    if (len > 2)
        result[len - 2] = '\0';

    free(copy);
    return result;
}

char *rewrite_css_urls(const char *html, const char *host)
{
    if (!html || !host)
        return NULL;

    size_t len = strlen(html);
    char *result = malloc(len * 2);
    if (!result)
        return NULL;

    size_t i = 0, j = 0;

    while (html[i])
    {
        if (strncmp(&html[i], "url(", 4) == 0)
        {
            i += 4;
            while (isspace(html[i]))
                i++;

            char quote = (html[i] == '"' || html[i] == '\'') ? html[i++] : '\0';
            size_t start = i;

            while (html[i] && html[i] != ')' && html[i] != quote)
                i++;

            size_t url_len = i - start;
            char url[1024];
            strncpy(url, &html[start], url_len);
            url[url_len] = '\0';

            char *rewritten = rewrite_attr("url", url, host);
            if (rewritten)
            {
                j += sprintf(&result[j], "url(\"%s\")", rewritten + 5); // skip url="
                free(rewritten);
            }

            if (html[i] == quote)
                i++;
            while (html[i] && html[i] != ')')
                i++;
            if (html[i] == ')')
                i++;
        }
        else
        {
            result[j++] = html[i++];
        }
    }

    result[j] = '\0';
    return result;
}

char *rewrite_all_html(const char *html, size_t html_len, const char *host)
{
    if (!html || !host)
        return NULL;

    char domain[512] = {0};
    normalize_domain(host, domain);

    // char *links = rewrite_links_inplace(html, domain);
    // if (!links) return NULL;

    // char *srcset = rewrite_srcset(links, domain);
    // free(links);
    // if (!srcset) return NULL;

    // char *css = rewrite_css_urls(srcset, domain);
    // free(srcset);
    // if (!css) return NULL;

    return inject_base_tag(html, html_len, domain);
}
