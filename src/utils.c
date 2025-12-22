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
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void set_handler(void (*f)(int), int sig_num) {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = f;
    sigemptyset(&act.sa_mask);

    if (sigaction(sig_num, &act, NULL) == -1) {
        if (errno == EINVAL) return;
        ERR("sigaction");
    }
}

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

        char quote_char = 0;
        char *start = p, *end = p;

        while(*p) {
            if(*p == '\\' && p[1] != '\0') {
                p++;
                *end++ = *p++; 
                continue;
            }

            if(*p == '"' || *p == '\'') {
                if(quote_char == 0) {
                    quote_char = *p;
                    p++;
                    continue;
                } else if(*p == quote_char) {
                    quote_char = 0;
                    p++;
                    continue;
                }
            }
            if(quote_char == 0 && isspace((unsigned char)*p)) break;
            
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
    return stat(path, &st) == 0;
}

int is_source_valid(const char* path) {
    struct stat st;
    if(stat(path, &st) != 0) {
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

int checked_mkdir(const char* path) {
    if(mkdir(path, 0755) != 0) {
        if(errno != EEXIST) {
            return -1;
        }

        struct stat s;
        if(stat(path, &s)) {
            return -1;
        }
        if(!S_ISDIR(s.st_mode)) {
            fprintf(stderr, "[ERROR] %s exists but is not a dir\n", path);
            return -1;
        }
    }

    return 0;
}

int make_path(const char* path) {
    char* tmp = strdup(path);
    if(!tmp) {
        free(tmp);
        return -1;
    }
    for(char *p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = '\0';
            if(checked_mkdir(tmp) == -1) {
                printf("[ERROR] failed checked_mkdir\n");
                return -1;
            }
            *p = '/';
        }
    }

    if(checked_mkdir(tmp)) {
        printf("[ERROR] failed checked_mkdir\n");
        return -1;
    }
    free(tmp);

    return 0;
}

char* get_abs_path(const char* path) {
    char* real = realpath(path, NULL);
    if(real) return real;

    if(path[0] == '/') {
        return strdup(path);
    }

    char *cwd = getcwd(NULL, 0);
    if(!cwd) {
        return NULL;
    }

    int n = strlen(cwd) + strlen(path) + 2;
    char* res = malloc(n); 
    if(!res) {
        free(cwd);
        return NULL;
    }

    snprintf(res, n, "%s/%s", cwd, path);
    free(cwd);
    
    return res;
}


int is_target_in_source(const char* src, const char* target) {
    char* abs_src = realpath(src, NULL);
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
            if(!last_slash) {
                free(tmp);
                free(abs_src);
                return -1;
            }
            if(last_slash == tmp) { 
                tmp[1] = '\0';
                abs_target = realpath(tmp, NULL);
                if(abs_target) {
                    break;
                }

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

void ensure_parent_dirs(const char* path) {
    char dir_path[PATH_MAX];
    int s = snprintf(dir_path, sizeof(dir_path), "%s", path);
    if(s < 0 || s >= (int)sizeof(dir_path)) {
        ERR("snprintf ensure_parent_dirs");
    }

    char* slash = strrchr(dir_path, '/');
    if(!slash) return;
    if(slash == dir_path) return;

    *slash = '\0';

    if(make_path(dir_path) == -1) {
        ERR("make_path");
    }
}