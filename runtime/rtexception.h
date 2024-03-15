#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>

typedef struct RtException
{
    char *ex_name;
    char *msg;
    bool GCFlag;
} RtException;

#define rtexception_free(exc) \
    if (exc)                  \
    {                         \
        free(exc->ex_name);   \
        free(exc->msg);       \
        free(exc);            \
    }
#define rtexception_print(exc) printf("%s.exception@%p", exc->ex_name, exc);

RtException *init_RtException(const char *exname, const char *msg);
RtException *rtexception_cpy(const RtException *exc);
char *rtexception_toString(const RtException *exc);
bool rtexception_compare(const RtException *exc1, const RtException *exc2);

/* All built in exceptions */

#define GenericExceptionString "Exception"
#define GenericException(msg) init_RtException(GenericExceptionString, msg);

#define InvalidTypeExceptionString "InvalidTypeException"
#define InvalidTypeException(msg) init_RtException(InvalidTypeExceptionString, msg)

#define InvalidNumberOfArgumentsExceptionString "InvalidNumberOfArgumentsException"
#define InvalidNumberOfArgumentsException(msg) init_RtException(InvalidNumberOfArgumentsExceptionString, msg)

#define ObjectNotCallableExceptionString "ObjectNotCallableException"
#define ObjectNotCallableException(msg) init_RtException(ObjectNotCallableExceptionString, msg)

#define NullPointerExceptionString "NullPointerException"
#define NullPointerException(msg) init_RtException(NullPointerExceptionString, msg)

#define UndefinedPointerExceptionString "UndefinedPointerException"
#define UndefinedPointerException(msg) init_RtException(UndefinedPointerExceptionString, msg)

#define IndexOutOfBoundsExceptionString "IndexOutOfBoundsException"
#define IndexOutOfBoundsException(msg) init_RtException(IndexOutOfBoundsExceptionString, msg)

#define KeyErrorExceptionString "KeyErrorException"
#define KeyErrorException(msg) init_RtException(KeyErrorExceptionString, msg)

#define NonIndexibleObjectExceptionString "NonIndexibleObjectException"
#define NonIndexibleObjectException(msg) init_RtException(NonIndexibleObjectExceptionString, msg)

#define DivisonByZeroExceptionString "DivisionByZeroException"
#define DivisonByZeroException(msg) init_RtException(DivisonByZeroExceptionString, msg)

#define NotImplementedExceptionString "NotImplementedException"
#define NotImplementedException(msg) init_RtException(NotImplementedExceptionString, msg)

#define StackOverflowExceptionString "StackOverflowException"
#define StackOverflowException(msg) init_RtException(StackOverflowExceptionString, msg)

#define InvalidAttributeExceptionString "InvalidAttributeException"
#define InvalidAttributeException(msg) init_RtException(InvalidAttributeExceptionString, msg)

#define InvalidValueExceptionString "InvalidValueException"
#define InvalidValueException(msg) init_RtException(InvalidValueExceptionString, msg)
