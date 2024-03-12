#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "../runtime/runtime.h"
#include "../runtime/rtexchandler.h"
#include "builtinexception.h"
#include "builtinfuncs.h"
#include "../generics/hashmap.h"
#include "../generics/utilities.h"


#define InsertBuiltIn(registry, builtin_struct) \
GenericHashMap_insert(registry, builtin_struct.builtin_name, (void *)&builtin_struct, false)

#define raiseArgumentCountException() \
raiseException(InvalidNumberOfArgumentsException("Exception Constructors expects 0 or 1 arguments"));

#define raiseInvalidTypeException() \
raiseException(InvalidNumberOfArgumentsException("Exception Constructors expects a String type"));

static RtObject *builtin_Exception(RtObject **args, int arg_count);
static RtObject *builtin_InvalidType(RtObject **args, int arg_count);
static RtObject *builtin_InvalidNumberOfArguments(RtObject **args, int arg_count);
static RtObject *builtin_NullPointerException(RtObject **args, int arg_count);

static const BuiltinFunc _builtin_Exception = {GenericExceptionString, builtin_Exception, 1};
static const BuiltinFunc _builtin_InvalidType = {InvalidTypeExceptionString, builtin_InvalidType, 1};
static const BuiltinFunc _builtin_InvalidNumberOfArguments = {InvalidNumberOfArgumentsExceptionString, builtin_InvalidNumberOfArguments, 1};
static const BuiltinFunc _builtin_NullPointerException = {NullPointerExceptionString, builtin_NullPointerException, 1};


/**
 * DESCRIPTION:
 * Initializes all built in exception and puts them into the builtin function registry
 * returns 1 -> initialization was successful
 * return 0 -> initialization failed
*/
int init_BuiltinException(GenericMap *BuiltinFunc_Registry) {
    assert(BuiltinFunc_Registry);

    InsertBuiltIn(BuiltinFunc_Registry, _builtin_Exception);
    InsertBuiltIn(BuiltinFunc_Registry, _builtin_InvalidType);
    InsertBuiltIn(BuiltinFunc_Registry, _builtin_InvalidNumberOfArguments);
    InsertBuiltIn(BuiltinFunc_Registry, _builtin_NullPointerException);

    return 1;
}


static RtObject *builtin_Exception(RtObject **args, int arg_count) {
    if(arg_count > 1) {
        raiseArgumentCountException()
    }
    if(arg_count == 1 && args[0]->type != STRING_TYPE) {
        raiseInvalidTypeException();
    }

    RtObject *obj = init_RtObject(EXCEPTION_TYPE);
    obj->data.Exception = GenericException(arg_count == 0? "": args[0]->data.String->string);
    return obj; 
}

static RtObject *builtin_InvalidType(RtObject **args, int arg_count) {
    if(arg_count > 1) {
        raiseArgumentCountException()
    }
    if(arg_count == 1 && args[0]->type != STRING_TYPE) {
        raiseInvalidTypeException();
    }

    RtObject *obj = init_RtObject(EXCEPTION_TYPE);
    obj->data.Exception = InvalidTypeException(arg_count == 0? "": args[0]->data.String->string);

    return obj;
}

static RtObject *builtin_InvalidNumberOfArguments(RtObject **args, int arg_count) {
    if(arg_count > 1) {
        raiseArgumentCountException()
    }
    if(arg_count == 1 && args[0]->type != STRING_TYPE) {
        raiseInvalidTypeException();
    }

    RtObject *obj = init_RtObject(EXCEPTION_TYPE);
    obj->data.Exception = InvalidNumberOfArgumentsException(arg_count == 0? "": args[0]->data.String->string);
    return obj;
}

static RtObject *builtin_NullPointerException(RtObject **args, int arg_count) {
    if(arg_count > 1) {
        raiseArgumentCountException()
    }
    if(arg_count == 1 && args[0]->type != STRING_TYPE) {
        raiseInvalidTypeException();
    }

    RtObject *obj = init_RtObject(EXCEPTION_TYPE);
    obj->data.Exception = NullPointerException(arg_count == 0? "": args[0]->data.String->string);
    return obj;
}



