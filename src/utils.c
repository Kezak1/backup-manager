#define _XOPEN_SOURCE 700
#include "utils.h"
#include <dirent.h>
#include <errno.h>
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
