#define _GNU_SOURCE
#include "utils.h"
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

char** split_string(const char* const input_string, int* count)
{
    char* input_copy = strdup(input_string);
    if (!input_copy)
        ERR("strdup");
    char* temp_output[256];
    *count = 0;
    char* token = strtok(input_copy, " ");
    while (token != NULL)
    {
        temp_output[(*count)++] = strdup(token);
        if (*count >= 256)
        {
            fprintf(stderr, "Too many components of the path (>256)! Aborting\n");
            exit(EXIT_FAILURE);
        }
        token = strtok(NULL, " ");
    }

    char** output_strings = malloc(sizeof(char*) * (*count));
    if (!output_strings)
        ERR("malloc");
    for (int i = 0; i < *count; i++)
    {
        output_strings[i] = temp_output[i];
    }

    free(input_copy);
    return output_strings;
}

void free_strings(char** strings, int count)
{
    for (int i = 0; i < count; i++)
    {
        free(strings[i]);
    }
    free(strings);
}

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

// void checked_mkdir(char* path) {
//     if(mkdir(path, 0755) != 0) {
//         if(errno != EEXIST) {
//             ERR("mkdir");
//         }

//         struct stat s;
//         if(stat(path, &s)) {
//             ERR("stat");
//         }
//         if(S_ISDIR(s.st_mode)) {
//             printf("%s is not a valid dir, lol\n", path);
//             exit(EXIT_SUCCESS);
//         }
//     }
// }

// void path_path(char* path) {
//     char* tmp = strdup(path);
//     if(!tmp) {
//         ERR("strdup");
//     }
//     for(char *p = tmp + 1; *p; p++) {
//         if(*p == '/') {
//             *p = '\0';
//             checked_mkdir(tmp);
//             *p = '/';
//         }
//     }
// }

