#pragma once

#include <stdbool.h>
typedef struct RtString {
    char* string;
    unsigned int length;
    size_t refcount;
} RtString;


RtString *init_RtString(const char* str);
void rtstr_free(RtString *string);
