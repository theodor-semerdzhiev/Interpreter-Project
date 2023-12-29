#pragma once
#include <stdbool.h>

char* cpy_string(const char* str);
unsigned int djb2_string_hash(const char *str);
unsigned int hash_int(const int *integer);
bool strings_equal(const char* str1, const char* str2);
bool integers_equal(const int *integer1, const int *integer2);
int get_pointer_list_length(void **args);

typedef bool (*IntFilter)(const int*);
IntFilter integer_bge_than(int cutoff);
