#pragma once
#include <sys/types.h>

#define MAX_BACKUP 16
#define MAX_TARGETS 16

typedef struct {
    char* source;
    char* targets[MAX_TARGETS];
    pid_t children_pids[MAX_TARGETS];
    int count;
} Backup;

typedef struct {
    Backup backups[MAX_BACKUP];
    int count;
} Backups;

void cmd_add(char** strs, int count, Backups *state);
void cmd_end(char** strs, int count, Backups *state);
void cmd_restore(char** strs, int count, Backups *state);
void cmd_list(Backups *state);
void clean_up_all(Backups *state);
