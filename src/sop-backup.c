#include <asm-generic/errno-base.h>
#define _GNU_SOURCE

#include "utils.h"
#include "commands.h"

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

void exit_handler(int sig_num) {
    puts("");
    sig_exit = 1;
}

void usage() {
    printf("USAGE:\n");
    printf("- adding backup: add <source path> <target path 1> <target path 2> ... <target path n>, n >= 1\n");
    printf("- ending backup: end <source path> <target path 1> <target path 2> ... <target path n>, n >= 1\n");
    printf("- restore source from target: restore <source path> <target path>\n");
    printf("- listing currect added backups: list\n");
    printf("- exiting the program: exit\n\n");
}

int main(void) {
    for(int sig_num = 1; sig_num < NSIG; sig_num++) {
        if (sig_num == SIGKILL || sig_num == SIGSTOP) continue;
        
        if(sig_num == SIGINT || sig_num == SIGTERM) {
            set_handler(exit_handler, sig_num);
        } else {
            set_handler(SIG_IGN, sig_num);
        }
    }

    Backups state;
    state.count = 0;

    printf("Welcome to the backup management system!\n\n");
    
    while(1) {
        printf("command: ");
        fflush(stdout);

        char* line = NULL;
        size_t len = 0;
        char** strs;
        int cnt = 0;

        if(getline(&line, &len, stdin) == -1) {
            free(line);
            clean_up_all(&state);
            break;
        };

        size_t l = strlen(line);
        if(l > 0 && line[l - 1] == '\n') {
            line[l - 1] = '\0';
        }
        strs = split_string(line, &cnt);

        if(cnt == 0) {
            free(line);
            free_strings(strs, cnt);
            usage();
            continue;
        }
   
        if(cnt == 1) {
            if(strcmp(strs[0], "exit") == 0) {
                free(line);
                free_strings(strs, cnt);
                clean_up_all(&state);
                break;

            } else if(strcmp(strs[0], "list") == 0) {
                cmd_list(&state);
            } else {
                usage();
            }
        } else if(cnt >= 3) {
            if(strcmp(strs[0], "add") == 0) {
                cmd_add(strs, cnt, &state);
            } else if(strcmp(strs[0], "end") == 0) {
                cmd_end(strs, cnt, &state);
            } else if(cnt == 3 && strcmp(strs[0], "restore") == 0) {
                cmd_restore(strs, cnt, &state);
            } else {
                usage();
            }
        } else {
            usage();
        }
        
        free(line);
        free_strings(strs, cnt);

        if(sig_exit) {
            clean_up_all(&state);
            break;
        }
    }

    return EXIT_SUCCESS;
}
