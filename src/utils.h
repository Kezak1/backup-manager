#pragma once
#include <sys/types.h>

char** split_string(const char* input_string, int* count);
void free_strings(char** strings, int count);
ssize_t bulk_read(int fd, char *buf, size_t count);
ssize_t bulk_write(int fd, char *buf, size_t count);
char* get_abs_path(const char* path);
int path_exist(const char *path);
int is_source_valid(const char* path);
int is_dir_empty(const char* path);
int is_target_in_source(const char* source, const char* target);
int checked_mkdir(char* path);
int make_path(char* path);


