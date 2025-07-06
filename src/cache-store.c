#include "../include/cache-store.h"

void ensure_cache_dir()
{
    struct stat st = {0};
    if (stat(CACHE_DIR, &st) == -1)
    {
        mkdir(CACHE_DIR, 0700);
    }
}

char *get_cache_filename(const char *url)
{
    size_t url_len = strlen(url);
    char *filename = malloc(url_len + 1);
    filename[url_len] = '\0';

    const char *invalid = "<>:\"/\\|?*";
    for (int i = 0; url[i]; i++)
    {
        unsigned char ch = url[i];
        if (ch < 32 || strchr(invalid, ch))
            filename[i] = '-';
        else
            filename[i] = url[i];
    }
}

int write_cache_file(const char *filename, const char *content_type, const char *data, size_t data_len)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path) - 1, "%s/%s", CACHE_DIR, filename);

    FILE *fptr = fopen(full_path, "wb");
    if (!fptr)
    {
        printf("failed to write file: %s\n", full_path);
        return 0;
    }

    // writing the content type with separator \n
    if (fprintf(fptr, "%s\n", content_type) <= 0)
    {
        printf("failed to write content type to file: %s\n", filename);
        return 0;
    }

    // writing the actual data
    if (fwrite(data, 1, data_len, fptr) != data_len)
    {
        printf("failed to write full data to file: %s\n", filename);
        return 0;
    }

    return 1;
}
char *read_cache_file(const char *filename, char *content_type_out, size_t *data_len_out)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path) - 1, "%s/%s", CACHE_DIR, filename);

    FILE *fptr = fopen(full_path, "rb");
    if (!fptr)
    {
        printf("failed to read file: %s\n", full_path);
        goto catch;
    }

    struct stat st;
    if (stat(fptr, &st) != 0)
        goto catch;
    long file_size = st.st_size;

    int ch = '\0';
    while ((ch = getc(fptr)) != '\n')
        *content_type_out++ = ch;
    *content_type_out = '\0';

    long body_start = ftell(fptr);
    if (body_start < 0 || body_start >= file_size)
        goto catch;

    long body_len = file_size - body_start;

    char *data = (char *)malloc(body_len + 1);
    if (!data)
        goto catch;

    size_t bytes_read = fread(data, 1, body_len, fptr);
    data[bytes_read] = '\0';
    *data_len_out = bytes_read;

    if (bytes_read != body_len)
    {
        printf("failed to read specified bytes from file: %s\n", filename);
        free(data);
        goto catch;
    }

    fclose(fptr);
    return data;

catch:
    if (fptr)
        fclose(fptr);
    return NULL;
}