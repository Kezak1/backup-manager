#include <asm-generic/errno-base.h>
#include <linux/limits.h>
#define _GNU_SOURCE

#include "utils.h"
#include "commands.h"

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

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
                if(s < 0 || s >= (int)sizeof(new_path)) {
                    ERR("snprintf");
                }

                (void)unlink(target);

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

int find_backup(Backup *b, int n, const char* src, const char* target) {
    for(int i = 0; i < n; i++) if(strcmp(b[i].source, src) == 0) {
        for(int j = 0; j < b[i].count; j++) if(strcmp(b[i].targets[j], target) == 0) {
            return 1;
        }
    }

    return 0;
}

void clean_up_chidren(Backup* b) {
    int n = b->count;
    for(int i = 0; i < n; i++) {
        if(b->children_pids[i] > 0) {
            if(kill(b->children_pids[i], SIGTERM) == -1) {
                ERR("kill");
            }
        }
    }

    for(int i = 0; i < n; i++) {
        if(b->children_pids[i] > 0) {
            waitpid(b->children_pids[i], NULL, 0);
        }
    }

    free(b->source);
    for(int i = 0; i < n; i++) {
        free(b->targets[i]);
    }
    b->count = 0;
}


void clean_up_all(Backups *state) {
    for(int i = 0; i < state->count; i++) {
        clean_up_chidren(&state->backups[i]);
    }
    state->count = 0;
}

void dfs(const char* src, const char* target, const char* abs_src, const char* abs_target) {
    DIR* dir = opendir(src);
    if(!dir) {
        ERR("opendir");
    }

    struct dirent *dp;
    while((dp = readdir(dir)) != NULL) {
        if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX], target_path[PATH_MAX];

        int s1 = snprintf(src_path, sizeof(src_path), "%s/%s", src, dp->d_name);
        int s2 = snprintf(target_path, sizeof(target_path), "%s/%s", target, dp->d_name);

        if(s1 < 0 || s1 >= (int)sizeof(src_path)) ERR("snprintf src_path");
        if(s2 < 0 || s2 >= (int)sizeof(target_path)) ERR("snprintf target_path");

        struct stat st;
        if(lstat(src_path, &st) == -1) {
            ERR("lstat");
        }

        if(S_ISDIR(st.st_mode)) {
            if(mkdir(target_path, st.st_mode) == -1) {
                if(errno != EEXIST){
                    ERR("mkdir");
                }
            }

            dfs(src_path, target_path, abs_src, abs_target);
        } else if(S_ISLNK(st.st_mode)) {
            copy_symlink(src_path, target_path, abs_src, abs_target);
        } else if(S_ISREG(st.st_mode)) {
            copy_file(src_path, target_path);
        }
    }

    if(closedir(dir) == -1) {
        ERR("closedir");
    }
}

void child_work(const char* abs_src, const char* abs_target) {
    if(!abs_src || !abs_target) {
        ERR("get_realpath");
    }
    printf("[PID: %d] starting copy from %s to %s\n", getpid(), abs_src, abs_target);
    dfs(abs_src, abs_target, abs_src, abs_target);
    printf("[PID: %d] done copy\n", getpid());
    exit(EXIT_SUCCESS);
}

void cmd_add(char** strs, int count, Backups *state) {
    char* src = strs[1];
    int n = count - 2;

    if(n > MAX_TARGETS) {
        fprintf(stderr, "[ERROR] too many targets, %d is max\n", MAX_TARGETS);
        return;
    }

    if(!is_source_valid(src)) {
        fprintf(stderr, "[ERROR] source is not valid directory\n");
        return;
    }

    char* abs_src = get_realpath(src);
    if(!abs_src) {
        ERR("get_realpath");
        return;
    }

    char* t[MAX_TARGETS];

    int valid = 1, allocated = 0;
    for(int i = 0; i < n && valid; i++) {
        char* target = strs[i + 2];

        if(path_exist(target)) {
            if(!is_dir_empty(target)) {
                fprintf(stderr, "[ERROR] target directory %s is not empty\n", target);
                valid = 0;
                break;
            }
        } else {
            path_path(target);
        }

        t[i] = get_realpath(target);
        if(!t[i]) {
            ERR("get_realpath");
        }

        allocated = i + 1;

        int check = is_target_in_source(src, target);
        if(check == -1) {
            fprintf(stderr, "[ERROR] cant resolve the target path\n");
            valid = 0;
            break;
        }
        if(check == 1) {
            fprintf(stderr, "[ERROR] target %s is inside source %s\n", target, src);
            valid = 0;
            break;
        }

        if(find_backup(state->backups, state->count, abs_src, t[i])) {
            fprintf(stderr, "[ERROR] backup from %s to %s already exists\n", abs_src, t[i]);
            valid = 0;
            break;
        }
    }

    if(valid == 0) {
        free(abs_src) ;
        for(int i = 0; i < allocated; i++) {
            free(t[i]);
        }

        return;
    }

    if(state->count >= MAX_BACKUP) {
        fprintf(stderr,"[ERROR] maximum number of backups has reached %d\n", MAX_BACKUP);
        free(abs_src);
        for(int i = 0; i < n; i++) {
            free(t[i]);
        }
        return;
    }

    Backup *b = &state->backups[state->count];
    b->source = abs_src;
    b->count = n;
    for(int i = 0; i < n; i++) {
        b->targets[i] = t[i];
    }

    pid_t pid;
    for(int i = 0; i < n; i++) {
        if((pid = fork()) == -1) {
            ERR("fork");
        }

        if(pid == 0) {
            child_work(abs_src, b->targets[i]);
            exit(EXIT_SUCCESS);
        }

        b->children_pids[i] = pid;
        printf("[INFO] started backup of %s for target %s (pid: %d)\n", abs_src, b->targets[i], pid);
    }

    state->count++;
    printf("[INFO] backup created, number of targets: %d\n", n);
    exit(EXIT_SUCCESS);
}   


void cmd_end(char** strs, int count, Backups *state) {
    ;
}
void cmd_restore(char** strs, int count, Backups *state) {
    ;
}
void cmd_list(Backups *state) {
    ;
}
