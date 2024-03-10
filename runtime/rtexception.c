#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../generics/utilities.h"
#include "rtexception.h"

/**
 * DESCRIPTION:
 * Initiates Runtime exception struct
*/
RtException *init_RtException(const char *exname, const char *msg) {
    RtException *exception = malloc(sizeof(RtException));
    if(!exception) return NULL;
    exception->ex_name = cpy_string(exname);
    exception->msg = cpy_string(msg);
    return exception;
}

/**
 * DESCRIPTION:
 * Performs a deep copy of the Exception
*/
RtException *rtexception_cpy(const RtException *exc) {
    assert(exc);
    RtException *cpy = malloc(sizeof(RtException));
    if(!cpy)
        MallocError();

    cpy->GCFlag=false;
    cpy->ex_name = cpy_string(exc->ex_name);
    cpy->msg = cpy_string(exc->msg);
    return cpy;
}

/**
 * DESCRIPTION:
 * Converts exception to string
*/
char *rtexception_toString(const RtException *exc) {
    assert(exc);
    assert(exc->ex_name);
    char buffer[100+strlen(exc->ex_name)];
    buffer[0] = '\0';
    snprintf(buffer, 100, "%s.exception@%p", exc->ex_name, exc);
    char *strcpy = cpy_string(buffer);
    if (!strcpy)
        MallocError();
    return strcpy;
}

/**
 * DESCRIPTION:
 * Logic for comparing runtime exceptions
*/
bool rtexception_compare(const RtException *exc1, const RtException *exc2) {
    assert(exc1);
    assert(exc2);
    if(exc1 == exc2) return true;
    return strings_equal(exc1->ex_name, exc2->ex_name);
}


