#define _GNU_SOURCE

#include "utils.h"

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>

#define FILE_BUF_LEN 256

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

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

void copy_symlink(const char* stc, const char* target) {
    
}

void dfs() {

}

void add() {
    
}
