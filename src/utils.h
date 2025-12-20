#pragma once
#include <sys/types.h>

char** split_string(const char* input_string, int* count);
void free_strings(char** strings, int count);
ssize_t bulk_read(int fd, char *buf, size_t count);
ssize_t bulk_write(int fd, char *buf, size_t count);
int path_exist(const char *path);
int is_source_valid(const char* path);
int is_dir_empty(const char* path);
void checked_mkdir(char* path);
void path_path(char* path);

