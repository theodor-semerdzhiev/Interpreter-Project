#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>

unsigned int djb2_string_hash(const char *str);
unsigned int hash_int(const int *integer);
bool strings_equal(const char* str1, const char* str2);
bool integers_equal(const int *integer1, const int *integer2);

typedef bool (*IntFilter)(const int*);
IntFilter integer_bge_than(int cutoff);
#endif