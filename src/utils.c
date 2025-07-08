#include "../include/utils.h"

// will read the full file and update the file_size provided var, file must be present
char *read_file(const char *file_path, size_t *file_size)
{
    // opening file in binary mode
    FILE *fptr = fopen(file_path, "rb");
    if (!fptr)
    {
        printf("failed to read file: %s\n", file_path);
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
    size_t bytes_written = fwrite(data, 1, data_size, fptr);

    if (bytes_written != data_size)
    {
        printf("failed to write given size of data to file: %s\n", file_path);
    }

    fclose(fptr);
}

// will return the no of minutes passed after the last modification of file
long time_passed_in_hours_for_file(const char *file_path)
{
    // taking stats of file
    struct stat st;
    stat(file_path, &st);

    // getting the current time
    time_t current_time = time(NULL);

    // returning the time passed in minutes
    return difftime(current_time, st.st_mtime) / 3600;
}

int urls_are_equivalent(const char *a, const char *b)
{
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);

    if (len_a > 0 && (a[len_a - 1] == '/' || a[len_a - 1] == '-'))
        len_a--;
    if (len_b > 0 && (b[len_b - 1] == '/' || b[len_b - 1] == '-'))
        len_b--;

    return len_a == len_b && strncmp(a, b, len_a) == 0;
}