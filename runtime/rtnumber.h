#pragma once
#include <stdbool.h>

typedef struct RtNumber {
    long double number;
    bool GCFlag;
} RtNumber;

typedef struct RtInteger {
    long integer;
    bool GCFlag;
} RtInteger;


#define rtnum_free(num) free(num);

RtNumber *init_RtNumber(long double number);
char *rtnumber_toString(const RtNumber *num);
