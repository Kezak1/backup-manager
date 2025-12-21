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

int find_target_in_source(Backup *b, int n, const char* src, const char* target) {
    for(int i = 0; i < n; i++) if(strcmp(b[i].source, src) == 0) {
        for(int j = 0; j < b[i].count; j++) if(strcmp(b[i].targets[j], target) == 0) {
            return 1;
        }
    }

    return 0;
}

int find_backup_by_source(Backups *state, const char *abs_src) {
    for(int i = 0; i < state->count; i++) {
        if(strcmp(state->backups[i].source, abs_src) == 0) return i;
    }

    return -1;
}

void clean_up_chidren(Backup* b) {
    int n = b->count;
    for(int i = 0; i < n; i++) {
        if(b->children_pids[i] > 0) {
            if(kill(b->children_pids[i], SIGTERM) == -1) {
                if(errno != ESRCH) {
                    ERR("kill");
                }
            }
        }
    }

    for(int i = 0; i < n; i++) {
        if(b->children_pids[i] > 0) {
            waitpid(b->children_pids[i], NULL, 0);
        }
    }

    if(b->source) {
        free(b->source);
        b->source = NULL;
    }
    for(int i = 0; i < n; i++) {
        if(b->targets[i]) {
            free(b->targets[i]);
            b->targets[i] = NULL;
        }
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
        ERR("realpath");
    }
    dfs(abs_src, abs_target, abs_src, abs_target);
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

    char* abs_src = realpath(src, NULL);

    char* t[MAX_TARGETS];
    char f[MAX_TARGETS] = {0}; // 0 - empty, C - to create, A - allocated 

    int valid = 1;
    for(int i = 0; i < n && valid; i++) {
        char* abs_target = get_abs_path(strs[i + 2]);
        if(!abs_target) {
            free(abs_target);
            free(abs_src);
            ERR("get_abs_path");
        }

        int check = is_target_in_source(src, abs_target);
        if(check == -1) {
            fprintf(stderr, "[ERROR] cant resolve the target path\n");
            free(abs_target);
            valid = 0;
            break;
        }
        if(check == 1) {
            fprintf(stderr, "[ERROR] target %s is inside source %s\n", abs_target, src);
            free(abs_target);
            valid = 0;
            break;
        }

        if(path_exist(abs_target)) {
            if(!is_dir_empty(abs_target)) {
                fprintf(stderr, "[ERROR] target directory %s is not empty\n", abs_target);
                free(abs_target);
                valid = 0;
                break;
            }
        } else {
            f[i] = 'C';   
        }

        free(abs_target);
    }
    
    if(valid == 0) {
        free(abs_src);
        return;
    }

    for(int i = 0; i < n && valid; i++) {
        char* abs_target = get_abs_path(strs[i + 2]);
        if(!abs_target) {
            free(abs_target);
            free(abs_src);
            ERR("get_abs_path");
        }
        if(f[i] == 'C') {
            if(make_path(strs[i + 2]) == -1) {
                free(abs_target);
                free(abs_src);
                ERR("make_path");   
            }
        }

        t[i] = realpath(abs_target, NULL);
        f[i] = 'A';
        free(abs_target);
        
        if(!t[i]) {
            valid = 0;
            break;
        }

        if(find_target_in_source(state->backups, state->count, abs_src, t[i])){
            fprintf(stderr, "[ERROR] target %s already exists for %s\n", t[i], abs_src);
            valid = 0;
            break;
        }

        for(int j = 0; j < i; j++) {
            if(strcmp(t[i], t[j]) == 0) {
                fprintf(stderr, "[ERROR] there is dublicate in input targets\n");
                valid = 0;
                break;
            }
        }
    }

    if(valid == 0) {
        free(abs_src) ;
        for(int i = 0; i < n; i++) {
            if(f[i] == 'A')
                free(t[i]);
        }

        return;
    }

    int idx = find_backup_by_source(state, abs_src);
    Backup *b = NULL;
    int is_new = 0;

    if(idx >= 0) {
        b = &state->backups[idx];
        free(abs_src);
        abs_src = b->source;
    } else {
        if(state->count >= MAX_BACKUP) {
            fprintf(stderr,"[ERROR] maximum number of backups has reached %d\n", MAX_BACKUP);
            free(abs_src);
            for(int i = 0; i < n; i++) {
                free(t[i]);
            }
            return;
        }

        b = &state->backups[state->count++];
        is_new = 1;
        b->source = abs_src;
        b->count = 0;

        for(int i = 0; i < n; i++) {
            b->children_pids[i] = -1;
        }
    }

    int start = b->count;
    if(b->count + n > MAX_TARGETS) {
        fprintf(stderr, "[ERROR] too many target directories (max %d)\n", MAX_TARGETS);
        for(int i = 0; i < n; i++) {
            free(t[i]);
        }

        if (is_new) {
            free(b->source);
            b->source = NULL;
            b->count = 0;
        }
        
        return;
    }
    
    for(int i = 0; i < n; i++) {
        b->targets[b->count + i] = t[i];
    }
    b->count += n;

    pid_t pid;
    for(int i = 0; i < n; i++) {
        int ti = start + i;
        if((pid = fork()) == -1) {
            ERR("fork");
        }

        if(pid == 0) {
            child_work(abs_src, b->targets[ti]);
            exit(EXIT_SUCCESS);
        }

        b->children_pids[ti] = pid;
        printf("[PID: %d] started backup of %s for target %s\n", pid, abs_src, b->targets[ti]);
    }

    printf("[INFO] backup created, number of targets: %d\n", n);
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
