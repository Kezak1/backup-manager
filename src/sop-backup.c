#define _GNU_SOURCE

#include "utils.h"

// #include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

void set_handler(void (*f)(int), int sig_num) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;

    if(-1 == sigaction(sig_num, &act, NULL)) {
        ERR("sigaction");
    }
}

void sig_exit_handler(int sig_num) {
    printf("\n");
    _exit(EXIT_SUCCESS);
}

void usage() {
    printf("wrong command bozo!\n");
}

int main(void) {
    for(int sig_num = 1; sig_num < NSIG; sig_num++) {
        if(sig_num == SIGKILL || sig_num == SIGSTOP) continue;
        
        if(sig_num == SIGINT || sig_num == SIGTERM) {
            set_handler(sig_exit_handler, sig_num);
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
            free_strings(strs, cnt_str);
            break;
        };

        size_t l = strlen(line);
        if(l > 0 && line[l - 1] == '\n') {
            line[l - 1] = '\0';
        }
        strs = split_string(line, &cnt_str);
        
        if(cnt_str == 1) {
            if(strcmp(strs[0], "exit") == 0) {
                free(line);
                free_strings(strs, cnt_str);
                exit(EXIT_SUCCESS);
            } else {
                usage();
            }
        } else if(cnt_str == 3) {
            if(strcmp(strs[0], "add") == 0) {
                
            }
            // for(int i = 0; i < cnt_str; i++) {
            //     printf("%s ", strs[i]);
            // }
            // printf("\n");
        } else {
            usage();
        }
        
        free(line);
        free_strings(strs, cnt_str);
    }
    return EXIT_SUCCESS;
}
