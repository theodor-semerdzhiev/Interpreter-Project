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

    cpy->GCFlag = false;
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
RtException *init_IndexOutOfBoundsException(size_t index, size_t length)
{
    char buffer[200];
    snprintf(buffer, sizeof(buffer),
             "Index out of bounds, cannot get index %zu of List with length %zu",
             index, length);
    RtException *exc = InvalidTypeException(buffer);
    return exc;
}

/**
 * DESCRIPTION:
 * Creates a key error exception
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
