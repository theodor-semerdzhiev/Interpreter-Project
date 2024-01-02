#pragma once
#include <stdbool.h>

typedef enum ErrorCode {
    FAILED_MEMORY_ALLOCATION = 1
} ErrorCode;

#define MallocError() exitprogram(FAILED_MEMORY_ALLOCATION)

void exitprogram(ErrorCode code);
char* cpy_string(const char* str);
unsigned int djb2_string_hash(const char *str);
unsigned int hash_int(const int *integer);
bool strings_equal(const char* str1, const char* str2);
bool integers_equal(const int *integer1, const int *integer2);
int get_pointer_list_length(void **arr);

typedef bool (*IntFilter)(const int*);
IntFilter integer_bge_than(int cutoff);

char* concat_strings(char *str1, char *str2);
