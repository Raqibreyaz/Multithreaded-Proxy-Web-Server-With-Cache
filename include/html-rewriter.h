#ifndef HTML_REWRITER_H
#define HTML_REWRITER_H
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

void normalize_domain(const char* input,char* new_input);
char *inject_base_tag(const char *html, size_t html_len, const char *host);
char *rewrite_attr(const char *attr, const char *value, const char *domain);
char *rewrite_links_inplace(const char *html, const char *host);
char *rewrite_srcset(const char *srcset, const char *host);
char *rewrite_css_urls(const char *html, const char *host);
char *rewrite_all_html(const char *html, size_t html_len, const char *host);
#endif