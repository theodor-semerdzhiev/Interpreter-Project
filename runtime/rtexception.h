#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>

typedef struct RtException {
    char *ex_name;
    char *msg;
    bool GCFlag;
} RtException;


#define rtexception_free(exc) free(exc->ex_name); free(exc);
#define rtexception_print(exc) printf("%s.exception@%p", exc->ex_name, exc);


RtException *init_RtException(const char *exname, const char *msg);
RtException *rtexception_cpy(const RtException *exc);
char *rtexception_toString(const RtException *exc);
bool rtexception_compare(const RtException *exc1, const RtException *exc2);

#define InvalidTypeException(msg) init_RtException("InvalidTypeException", msg)
#define InvalidNumberOfArgumentsException(msg) init_RtException("InvalidNumberOfArgumentsException", msg)
#define NullPointerException(msg) init_RtException("NullPointerException", msg)
#define UndefinedPointerException(msg) init_RtException("UndefinedPointerException", msg)
#define IndexOutOfBoundsException(msg) init_RtException("IndexOutOfBoundsException", msg)
#define KeyErrorException(msg) init_RtException("KeyErrorException", msg)
#define DivisonByZeroException(msg) init_RtException("DivisionByZeroException", msg)
#define NotImplementedException(msg) init_RtException("NotImplementedException", msg)
#define StackOverflowException(msg) init_RtException("StackOverflowException", msg)
#define InvalidAttributeException(msg) init_RtException("InvalidAttributeException", msg);
#define InvalidValueException(msg) init_RtException("InvalidValueException", msg);
