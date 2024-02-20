#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "rtnumber.h"
#include "../generics/utilities.h"


/**
 * DESCRIPTION:
 * Initializes rt number struct
*/
RtNumber *init_RtNumber(long double number) {
    RtNumber *num = malloc(sizeof(RtNumber));
    if(!num) MallocError();
    num->number=number;
    num->GCFlag=false;
    return num;
}

/**
 * DESCRIPTION:
 * Converts rt number to string
*/
char *rtnumber_toString(const RtNumber *num) {
    assert(num);
    char buffer[124];
    snprintf(buffer, sizeof(buffer), "%Lf", num->number);
    return cpy_string(buffer);
}
