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
#include <ctype.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

char** split_string(const char* input_string, int* count)
{
    char* input_copy = strdup(input_string);
    if (!input_copy)
        ERR("strdup");

    char** output_strings = NULL;
    *count = 0;
    int cap = 0;

    char *p = input_copy;
    while(*p) {
        while(isspace((unsigned char)*p)) p++;
        if(!*p) break;

        int in_quotes = 0;
        char *start = p, *end = p;

        while(*p) {
            if(*p == '\\' && p[1] != '\0') {
                p++;
                *end++ = *p++; 
                continue;
            }

            if(*p == '"') {
                in_quotes = 1 - in_quotes;
                p++;
                continue;
            }
            if(in_quotes == 0 && isspace((unsigned char)*p)) break;
            
            *end++ = *p++;
        }
        int had_delim = (*p != '\0');
        *end = '\0';

        if(*count == cap) {
            if(cap) {
                cap *= 2;
            } else {
                cap = 4;
            }
            output_strings = realloc(output_strings, cap * sizeof(char *));
            if(!output_strings) ERR("realloc");
        }

        output_strings[*count] = strdup(start);
        if(!output_strings[*count]) {
            ERR("strdup");
        }
        
        (*count)++;
        if (had_delim) p++; 
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
            return len;
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

int path_exist(const char *path) {
    struct stat st;
    return lstat(path, &st) == 0;
}

int is_source_valid(const char* path) {
    struct stat st;
    if(lstat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

int is_dir_empty(const char* path) {
    DIR* dir = opendir(path);
    if(!dir) {
        return 0;
    }

    struct dirent *dp;
    errno = 0;

    while((dp = readdir(dir)) != NULL) {
        if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        
        if(closedir(dir)) {
            ERR("closedir");
        }
        return 0;
    }

    if(errno != 0) {
        ERR("readdir");
    }
    if(closedir(dir)) {
        ERR("closedir");
    }   
    return 1;
}

void checked_mkdir(char* path) {
    if(mkdir(path, 0755) != 0) {
        if(errno != EEXIST) {
            ERR("mkdir");
        }

        struct stat s;
        if(stat(path, &s)) {
            ERR("stat");
        }
        if(!S_ISDIR(s.st_mode)) {
            fprintf(stderr, "%s exists but is not a dir\n", path);
            exit(EXIT_FAILURE);
        }
    }
}

void path_path(char* path) {
    char* tmp = strdup(path);
    if(!tmp) {
        ERR("strdup");
    }
    for(char *p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = '\0';
            checked_mkdir(tmp);
            *p = '/';
        }
    }

    checked_mkdir(tmp);
    free(tmp);
}

char* get_realpath(const char* path) {
    char* real = realpath(path, NULL);
    if(!real) {
        return NULL;
    }
    return real;
}

int is_target_in_source(const char* src, const char* target) {
    char* abs_src = realpath(src, NULL);
    if(!abs_src) {
        ERR("realpath");
    }

    char* abs_target = realpath(target, NULL);

    if(!abs_target) {
        char* tmp = strdup(target);
        if(!tmp) {
            free(abs_src);
            ERR("strdup");
        }

        while(1) {
            abs_target = realpath(tmp, NULL);
            if(abs_target) {
                break;
            }

            char* last_slash = strrchr(tmp, '/');
            if(!last_slash || last_slash == tmp) {
                free(tmp);
                free(abs_src);
                return -1;
            }

            *last_slash = '\0';
        }
        free(tmp);
    }

    size_t src_len = strlen(abs_src);
    size_t target_len = strlen(abs_target);

    int res = 0;

    if(target_len >= src_len) {
        if(strncmp(abs_target, abs_src, src_len) == 0) {
            if(target_len == src_len || abs_target[src_len] == '/') {
                res = 1;
            }
        }
    }

    free(abs_src);
    free(abs_target);
    return res;
}

