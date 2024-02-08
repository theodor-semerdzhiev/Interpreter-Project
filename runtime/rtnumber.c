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
