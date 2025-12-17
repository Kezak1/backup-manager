#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage() {
    ;
}

int main(int argc, const char* argv[]) {
    printf("Hello to the backup management system by Kezak1\n");
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
