#define _GNU_SOURCE

#include "utils.h"
#include "commands.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

volatile sig_atomic_t sig_exit = 0;

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
void cleanup_handler(int sig_num) {
    sig_exit = 1;
}

void usage() {
    printf("USAGE:\n");
    printf("- adding backup: add <source path> <target path 1> <target path 2> ... <target path n>, n >= 1\n");
    printf("- ending backup: end <source path> <target path>\n");
    printf("- restore source from target: restore <source path> <target path>\n");
    printf("- listing currect added backups: list\n");
    printf("- exiting the program: exit\n\n");
}

int main(void) {
    for(int sig_num = 1; sig_num < NSIG; sig_num++) {
        if (sig_num == SIGKILL || sig_num == SIGSTOP || sig_num == SIGCHLD) continue;
        
        if(sig_num == SIGINT || sig_num == SIGTERM) {
            set_handler(cleanup_handler, sig_num);
        } else {
            set_handler(SIG_IGN, sig_num);
        }
    }

    printf("Welcome to the backup management system!\n\n");
    
    while(1) {
        printf("command: ");

        char* line = NULL;
        size_t len = 0;
        char** strs;
        int cnt_str = 0;
        if(getline(&line, &len, stdin) == -1) {
            free(line);
            break;
        };

        size_t l = strlen(line);
        if(l > 0 && line[l - 1] == '\n') {
            line[l - 1] = '\0';
        }
        strs = split_string(line, &cnt_str);
        
        if(cnt_str == 1) {
            if(strcmp(strs[0], "exit") == 0) {
                cleanup_handler(0);
            } else {
                usage();
            }
        }
        else if(cnt_str > 2) {;
            if(strcmp(strs[0], "add") == 0) {
                cmd_add(strs, cnt_str);
            }
        } 
        else {
            usage();
        }
        
        free(line);
        free_strings(strs, cnt_str);

        if(sig_exit) {
            break;
        }
    }
    return EXIT_SUCCESS;
}
