#include <errno.h>
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
    puts("");
    _exit(EXIT_SUCCESS);
}

void usage() {
    ;
}

int main() {
    set_handler(sig_exit_handler, SIGINT);
    set_handler(sig_exit_handler, SIGTERM);

    printf("Welcome to the backup management system!\n\n");
    while(1) {
        printf("command: ");
        char cmd[100];
        scanf("%s", cmd);
        
        if(strcmp(cmd, "exit") == 0) {
            break;
        }
    }
    return EXIT_SUCCESS;
}
