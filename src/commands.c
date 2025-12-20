#define _GNU_SOURCE

#include "utils.h"

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
// #include <signal.h>
#include <stdlib.h>
#include <string.h>

#define FILE_BUF_LEN 256

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void copy_file(const char* src, const char* target) {
    int fd_src = open(src, O_RDONLY);
    if(fd_src < 0) {
        ERR("open");
    }

    int fd_target = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd_target < 0) {
        if(close(fd_src) < 0) {
            ERR("close");
        }
        ERR("open");
    }

    char file_buf[FILE_BUF_LEN];
    while(1) {
        const ssize_t read_size = bulk_read(fd_src, file_buf, FILE_BUF_LEN);
        if(read_size == -1) {
            ERR("bulk_read");
        }

        if(read_size == 0) {
            break;
        }

        if(bulk_write(fd_target, file_buf, read_size) == -1) {
            ERR("bulk_write");
        }
    }

    if(close(fd_target) == -1) {
        ERR("close");
    }
    if(close(fd_src) == -1) {
        ERR("close");
    }
}

void copy_symlink(const char* src, const char* target, const char* abs_src, const char* abs_target) {
    char path[PATH_MAX];
    ssize_t len = readlink(src, path, sizeof(path) - 1);    
    if(len == -1) {
        ERR("readlink");
    }
    path[len] = '\0';

    size_t abslen = strlen(abs_src);

    if(path[0] == '/') {
        if(strncmp(path, abs_src, abslen) == 0) {
            if(path[abslen] == '\0' || path[abslen] == '/') {
                char new_path[PATH_MAX];

                int s = snprintf(new_path, sizeof(new_path), "%s%s", abs_target, path + abslen);
                if(s >= (int)sizeof(new_path)) {
                    fprintf(stderr, "[ERROR] symlink path too long\n");
                    return;
                }
                if(s < 0) {
                    ERR("snprintf");
                }

                (void)unlink;

                if(symlink(new_path, target) == -1) {
                    ERR("symlink");
                } 

                return;
            }
        }
    }

    (void)unlink(target);
    if(symlink(path, target) == -1) {
        ERR("symlink");
    }
}

void dfs() {
    ;
}

void cmd_add(char** strs, int count) {
    char* src = strs[1];

    if(!is_source_valid(src)) {
        fprintf(stderr, "[ERROR] source is not valid directory\n");
        return;
    }

    for(int i = 2; i < count; i++) {
        char* target = strs[i];

        int check = is_target_in_source(src, target);
        if(check == -1) {
            fprintf(stderr, "[ERROR] cant resolve the target path\n");
            return;
        }
        if(check == 1) {
            fprintf(stderr, "[ERROR] target %s is inside source %s\n", target, src);
            return;
        }

        if(path_exist(target)) {
            if(!is_dir_empty(target)) {
                fprintf(stderr, "[ERROR] target directory %s is not empty\n", target);
                return;
            }
        }
    }

    printf("LOL ADD\n");
}