#ifndef UTILS_H
#define UTILS_H

char** split_string(const char* const input_string, int* count);

void free_strings(char** strings, int count);

#endif