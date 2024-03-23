#pragma once
#include "rtobjects.h"
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

RtException *init_InvalidRaiseTypeException(const RtObject *raisedobj);

RtException *init_InvalidAttrsException(const RtObject *target, const char *attrname);

RtException *init_NonIndexibleObjectException(const RtObject *nonindexibleobj);

RtException *init_InvalidIndexTypeException(
    const RtObject *index, 
    const RtObject *target, 
    const char *expected_index_type);

RtException *init_IndexOutOfBoundsException(size_t index, size_t length);

RtException *init_KeyErrorException(const RtObject *target, const RtObject *key);

RtException *init_InvalidTypeException_BinaryOp(
    const RtObject *obj1, 
    const RtObject *obj2, 
    const char* binaryOp);

RtException *init_InvalidTypeException_UnaryOp(const RtObject *target, const char *unaryOp);

RtException *init_InvalidNumberOfArgumentsException(const char* callable_name, size_t actual_args, size_t expected_args);

RtException *init_InvalidTypeException_Builtin(const char* builtin_name, const char* expected_type, const RtObject *actual_arg);


/* All built in exceptions */

#define GenericExceptionString "Exception"
#define GenericException(msg) init_RtException(GenericExceptionString, msg);

#define InvalidTypeExceptionString "InvalidTypeException"
#define InvalidTypeException(msg) init_RtException(InvalidTypeExceptionString, msg)

#define InvalidNumberOfArgumentsExceptionString "InvalidNumberOfArgumentsException"
#define InvalidNumberOfArgumentsException(msg) init_RtException(InvalidNumberOfArgumentsExceptionString, msg)

#define ObjectNotCallableExceptionString "ObjectNotCallableException"
#define ObjectNotCallableException(msg) init_RtException(ObjectNotCallableExceptionString, msg)

#define NullTypeExceptionString "NullTypeException"
#define NullTypeException(msg) init_RtException(NullTypeExceptionString, msg)

#define UndefinedTypeExceptionString "UndefinedTypeException"
#define UndefinedTypeException(msg) init_RtException(UndefinedTypeExceptionString, msg)

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

#define IOExceptionString "IOException"
#define IOExceptionException(msg) init_RtException(IOExceptionString, msg)
