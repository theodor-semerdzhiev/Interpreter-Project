#pragma once
#include <stdbool.h>
#include <ctype.h>

typedef enum ErrorCode
{
    FAILED_MEMORY_ALLOCATION = 1,
    FAILED_BUILTINS_INIT = 2
} ErrorCode;

#define MallocError() exitprogram(FAILED_MEMORY_ALLOCATION)

bool is_token_integer(char *token);
bool is_token_numeric(char *token);
void exitprogram(ErrorCode code);
char *cpy_string(const char *str);
char* append_char(const char *str, char c);
char *surround_string(const char *str, size_t strlen, char start, char end);
unsigned int djb2_string_hash(const char *str);
unsigned int hash_pointer(const void* ptr);
unsigned int hash_int(const int *integer);
unsigned int murmurHashUInt(double key);
bool strings_equal(const char *str1, const char *str2);
bool integers_equal(const int *integer1, const int *integer2);
bool ptr_equal(const void* ptr1, const void* ptr2);
int get_pointer_list_length(void **arr);

typedef bool (*IntFilter)(const int *);
IntFilter integer_bge_than(int cutoff);

char *concat_strings(char *str1, char *str2);
char *malloc_substring(const char *str, int start, int end);
