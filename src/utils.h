#pragma once
#include <sys/types.h>

char** split_string(const char* const input_string, int* count);
void free_strings(char** strings, int count);
ssize_t bulk_read(int fd, char *buf, size_t count);
ssize_t bulk_write(int fd, char *buf, size_t count);