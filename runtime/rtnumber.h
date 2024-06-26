#pragma once
#include <stdbool.h>

typedef struct RtNumber {
    long double number;
    size_t refcount;
} RtNumber;

typedef struct RtInteger {
    long integer;
    size_t refcount;
} RtInteger;


#define rtnum_free(num) free(num);

RtNumber *init_RtNumber(long double number);
char *rtnumber_toString(const RtNumber *num);
