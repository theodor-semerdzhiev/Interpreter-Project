#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "rtstring.h"
#include "../generics/utilities.h"


/**
 * DEESCRIPTION:
 * Initializes a RtString struct, input str is malloced
 * 
 * NOTE:
 * Returns NULL if malloc fails
 * 
 * PARAMS:
 * str: string
 * 
*/
RtString *init_RtString(const char* str) {
    RtString *rtstring = malloc(sizeof(RtString));
    if(!rtstring) return NULL;
    rtstring->string = str? cpy_string(str): NULL;
    if(!rtstring->string) {
        free(rtstring);
        return NULL;
    }
    rtstring->length = str? strlen(str): 0;
    rtstring->GCFlag=false;
    return rtstring;
}

/**
 * DESCRIPTION:
 * Frees RtString 
*/
void rtstr_free(RtString *string) {
    if(!string) return;
    free(string->string);
    free(string);
}