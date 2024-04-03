#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../generics/utilities.h"
#include "rtexception.h"
#include "rtobjects.h"

/**
 * DESCRIPTION:
 * Initiates Runtime exception struct
 */
RtException *init_RtException(const char *exname, const char *msg)
{
    RtException *exception = malloc(sizeof(RtException));
    if (!exception)
        MallocError();
    exception->ex_name = cpy_string(exname);
    exception->msg = cpy_string(msg);
    exception->refcount = 0;
    return exception;
}

/**
 * DESCRIPTION:
 * Performs a deep copy of the Exception
 */
RtException *rtexception_cpy(const RtException *exc)
{
    assert(exc);
    RtException *cpy = malloc(sizeof(RtException));
    if (!cpy)
        MallocError();

    cpy->ex_name = cpy_string(exc->ex_name);
    cpy->msg = cpy_string(exc->msg);
    return cpy;
}

/**
 * DESCRIPTION:
 * Converts exception to string
 */
char *rtexception_toString(const RtException *exc)
{
    assert(exc);
    assert(exc->ex_name);
    char buffer[100 + strlen(exc->ex_name)];
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
bool rtexception_compare(const RtException *exc1, const RtException *exc2)
{
    assert(exc1);
    assert(exc2);
    if (exc1 == exc2)
        return true;
    return strings_equal(exc1->ex_name, exc2->ex_name);
}

/**
 * DESCRIPTION:
 * Creates a InvalidTypeException
 *
 * USECASE:
 * Attempting to raise an exception but an object with an Exception Type was not provided
 *
 * PARAMS:
 * raisedobj: type given to raise condition
 */
RtException *init_InvalidRaiseTypeException(const RtObject *raisedobj)
{
    assert(raisedobj);

    const char *type = rtobj_type_toString(raisedobj->type);
    char *objstr = rtobj_toString(raisedobj);

    char buffer[strlen(type) + 100 + strlen(objstr)];

    snprintf(buffer, sizeof(buffer),
             "Cannot raise the Object %s with type %s. Raise conditions must always be a Exception type.",
             objstr, type);

    free(objstr);

    RtException *exc = InvalidTypeException(buffer);
    return exc;
}

/**
 * DESCRIPTION:
 * Creates a InvalidAttributeException
 *
 * USECASE:
 * Trying to fetch an attribute that does not exist on a object.
 *
 * PARAMS:
 * target: target object that attribute was attempted to be fetched from
 * attrname: name of the attribute
 */
RtException *init_InvalidAttrsException(const RtObject *target, const char *attrname)
{
    assert(target && attrname);

    const char *type = rtobj_type_toString(target->type);
    char *objtostr = rtobj_toString(target);

    char buffer[strlen(objtostr) + strlen(type) + strlen(attrname) + 60];

    snprintf(buffer, sizeof(buffer),
             "Object %s with type %s does not have attribute '%s'",
             objtostr, type, attrname);

    free(objtostr);

    RtException *exc = InvalidAttributeException(buffer);
    return exc;
}

/**
 * DESCRIPTION:
 * Creates a NonIndexibleObjectException
 *
 * USECASE:
 * Trying to fetch an index (or key) on an object that does not support indexing
 *
 * PARAMS:
 * nonindexibleobj: object that cannot be indexed
 *
 */
RtException *init_NonIndexibleObjectException(const RtObject *nonindexibleobj)
{
    assert(nonindexibleobj);

    const char *type = rtobj_type_toString(nonindexibleobj->type);
    char *obj_tostring = rtobj_toString(nonindexibleobj);

    char buffer[50 + strlen(type) + strlen(obj_tostring)];
    snprintf(buffer, sizeof(buffer), "Object %s with type %s, is not indexible", obj_tostring, type);

    free(obj_tostring);

    RtException *exc = NonIndexibleObjectException(buffer);
    return exc;
}

/**
 * DESCRIPTION:
 * Creates a InvalidTypeException
 *
 * USECASE:
 * Example: Trying to fetch an index with type of String on a list
 *
 * PARAMS:
 * index: index object
 * target: target of the object
 * expected_index_type: expected type of the index obj
 *
 */
RtException *init_InvalidIndexTypeException(
    const RtObject *index,
    const RtObject *target,
    const char *expected_index_type)
{
    assert(index);
    assert(target);
    assert(expected_index_type);

    const char *targettype = rtobj_type_toString(target->type);
    char *indextostr = rtobj_toString(index);
    char *targettostr = rtobj_toString(target);

    char buffer[100 + strlen(indextostr) + strlen(targettype) + strlen(targettostr)];

    snprintf(buffer, sizeof(buffer),
             "Invalid index Object %s. Target Object %s with type %s must take an index of type %s",
             indextostr, targettostr, targettype, expected_index_type);

    free(indextostr);
    free(targettostr);

    RtException *exc = InvalidTypeException(buffer);
    return exc;
}

/**
 * DESCRIPTION:
 * Creates a IndexOutOfBoundsException, only involving lists
 *
 * USECASE:
 * Fetching an index 10 of a list of size 1
 *
 * PARAMS:
 * index: attempted index to be fetched
 * length: length of list
 *
 */
RtException *init_IndexOutOfBoundsException(const RtObject *list, size_t index, size_t length)
{   
    assert(list);
    assert(list->type == LIST_TYPE);

    char *listtostr = rtobj_toString(list);
    char buffer[200+strlen(listtostr)];

    snprintf(buffer, sizeof(buffer),
             "Index out of bounds, cannot get index %zu of List Object %s with length %zu",
             index, listtostr, length);
    
    free(listtostr);
    RtException *exc = InvalidTypeException(buffer);
    return exc;
}

/**
 * DESCRIPTION:
 * Creates a KeyErrorException
 *
 * USECASE:
 * Example: Used when trying to fetch something from a set or map but the key does not exist
 *
 * PARAMS:
 * target:
 * key:
 */
RtException *init_KeyErrorException(const RtObject *target, const RtObject *key)
{
    assert(target);
    assert(key);

    const char *keytype = rtobj_type_toString(key->type);
    const char *targettype = rtobj_type_toString(target->type);

    char *keytostr = rtobj_toString(key);
    char *targettostr = rtobj_toString(target);

    char buffer[
        75 + strlen(keytype) + strlen(targettype) + strlen(keytostr) + strlen(targettostr)
    ];

    snprintf(buffer, sizeof(buffer),
             "Key Object %s with type %s does not exist on Object %s with type %s",
             keytostr, keytype, targettostr, targettype);

    free(targettostr);
    free(keytostr);

    RtException *exc = KeyErrorException(buffer);
    return exc;
}

/**
 * DESCRIPTION:
 * Creates a InvalidTypeException for binary operations
 * 
 * USECASE:
 * Used when types do not match for a certain binary or unary operation
 * 
 * PARAMS:
*/
RtException *init_InvalidTypeException_BinaryOp(
    const RtObject *obj1, 
    const RtObject *obj2, 
    const char* binaryOp) {
    
    assert(obj1); 
    assert(obj2); 
    assert(binaryOp);

    const char* typeobj1 = rtobj_type_toString(obj1->type);
    const char* typeobj2 = rtobj_type_toString(obj2->type);
    char *obj1tostr = rtobj_toString(obj1);
    char *obj2tostr = rtobj_toString(obj1);

    char buffer[
        100 + strlen(binaryOp) + strlen(typeobj1) + strlen(typeobj2) + strlen(obj1tostr) + strlen(obj2tostr)
    ];

    snprintf(buffer, sizeof(buffer),
        "Cannot perform %s operation on Objects %s and %s, with type %s and %s, respectively.",
        binaryOp, obj1tostr, obj2tostr, typeobj1, typeobj2
    );

    free(obj1tostr);
    free(obj2tostr);

    RtException *exc = InvalidTypeException(buffer);
    return exc;
}

/**
 * DESCRIPTION:
 * Creates a InvalidTypeException for cases where a invalid type was given to a unary operator
 * 
 * PARAMS:
 * target: object that unary operator was applied to
 * unaryOp: Name of the operation
*/
RtException *init_InvalidTypeException_UnaryOp(const RtObject *target, const char *unaryOp) {
    assert(target);
    assert(unaryOp);

    const char *target_type = rtobj_type_toString(target->type);

    char* targettostr = rtobj_toString(target);

    char buffer[100 + strlen(unaryOp) + strlen(target_type) + strlen(targettostr)];

    snprintf(buffer, sizeof(buffer),
        "Cannot perform %s unary operation on Object %s with type %s",
        unaryOp, targettostr, target_type
    );

    free(targettostr);

    RtException *exc = InvalidTypeException(buffer);
    return exc;
}

/**
 * DESCRIPTION:
 * Creates a InvalidNumberOfArgumentsException
 * 
 * PARAMS:
 * callable_name: name of object that was called with given arguments
 * actual_args: number of arguments that was given to callable object
 * expected_args: number of arguments that the callable object was expecting 
 * 
 * NOTE:
 * If expected_args == INT64_INT, then function assumes callable object must take any amount of arguments (but at least more than 1)
*/
RtException *init_InvalidNumberOfArgumentsException(const char* callable_name, size_t actual_args, size_t expected_args) {

    assert(callable_name);

    char buffer[300 + strlen(callable_name)];

    if(expected_args != INT64_MAX) {
        snprintf(buffer, sizeof(buffer),
            "%s expects %zu arguments, but got %zu arguments.",
            callable_name, expected_args, actual_args
        );
    } else {
        snprintf(buffer, sizeof(buffer),
            "%s expects more than 0 arguments, but got %zu arguments.",
            callable_name, actual_args
        );
    }

    RtException *exc = InvalidTypeException(buffer);
    return exc;
}


/**
 * DESCRIPTION:
 * Creates a InvalidTypeException
 * 
 * USECASE:
 * When a builtin function expects a certain object type but gets an other
 * 
 * PARAMS:
 * builtin_name: name of the builtin function
 * expected_type: the type the builtin function expects
 * actual_arg: the invalid argument
*/
RtException *init_InvalidTypeException_Builtin(const char* builtin_name, const char* expected_type, const RtObject *actual_arg) {

    assert(builtin_name);
    assert(expected_type);
    assert(actual_arg);

    const char* actual_type = rtobj_type_toString(actual_arg->type);
    char *argtostr = rtobj_toString(actual_arg);

    char buffer[
        90 + strlen(builtin_name) + strlen(expected_type) + strlen(actual_type) + strlen(argtostr)
    ];

    snprintf(buffer, sizeof(buffer),
        "Builtin function %s expected argument with type %s, but got Object %s with type %s",
        builtin_name, expected_type, argtostr, actual_type
    );

    free(argtostr);

    RtException *exc = InvalidTypeException(buffer);
    return exc;
}
