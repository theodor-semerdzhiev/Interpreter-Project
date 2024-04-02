#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "rtobjects.h"


/**
 * DESCRIPTION:
 * Returns wether the type is primitive
*/
bool rttype_isprimitive(RtType type) {
    return type == NUMBER_TYPE || type == NULL_TYPE || type == UNDEFINED_TYPE;
}

/**
 * DESCRIPTION:
 * Helper function for getting name of a type
 */
const char *rtobj_type_toString(RtType type)
{
    switch (type)
    {
    case UNDEFINED_TYPE:
        return "Undefined";
    case NULL_TYPE:
        return "Null";
    case NUMBER_TYPE:
        return "Number";
    case STRING_TYPE:
        return "String";
    case CLASS_TYPE:
        return "Class";
    case FUNCTION_TYPE:
        return "Function";
    case LIST_TYPE:
        return "List";
    case HASHMAP_TYPE:
        return "Map";
    case HASHSET_TYPE:
        return "Set";
    case EXCEPTION_TYPE:
        return "Exception";
    }
}

/**
 * DESCRIPTION:
 * Given on the type it will free the pointer properly
 * 
 * PARAMS:
 * type: type of the data pointer
 * data: pointer to free
 * freerefs: wether associated objects should also be freed
 * update_ref_counts: wether references counts should be updated
*/
void rttype_freedata(RtType type, void *data, bool freerefs, bool update_ref_counts) {
    assert(data);
    switch (type)
    {
    case NULL_TYPE:
        free(data);
        return;
    case UNDEFINED_TYPE:
        free(data);
        return;
    case NUMBER_TYPE:
        rtnum_free(data);
        return;
    case STRING_TYPE:
        rtstr_free(data);
        return;
    case LIST_TYPE:
        rtlist_free((RtList*)data, freerefs, update_ref_counts);
        return;
    case FUNCTION_TYPE: 
        rtfunc_free((RtFunction*) data, false, update_ref_counts);
        return;
    case HASHMAP_TYPE:
        rtmap_free((RtMap*)data, freerefs, freerefs, false, update_ref_counts);
        return;
    case HASHSET_TYPE:
        rtset_free((RtSet*)data, freerefs, false, update_ref_counts);
        return;
    case CLASS_TYPE:
        rtclass_free((RtClass*)data, freerefs, false, update_ref_counts);
        return;
    case EXCEPTION_TYPE:
        rtexception_free(((RtException*)data));
    }
}

/**
 * DESCRIPTION:
 * Sets the GCFlag
*/
void rttype_set_GCFlag(void* data, RtType type, bool flag) {
    assert(data);
    switch (type)
    {
        case NULL_TYPE:
        case UNDEFINED_TYPE:
            *((bool*)data) = flag;
            return;
        case NUMBER_TYPE:
            ((RtNumber*)data)->GCFlag = flag;
            return;
        case STRING_TYPE:
            ((RtString*)data)->GCFlag = flag;
            return;
        case LIST_TYPE:
            ((RtList*)data)->GCFlag = flag;
            return;
        case FUNCTION_TYPE:
            ((RtFunction*)data)->GCFlag = flag;
            return;
        case HASHMAP_TYPE:
            ((RtMap*)data)->GCFlag = flag;
            return;
        case CLASS_TYPE:
            ((RtClass*)data)->GCFlag = flag;
            return;
        case HASHSET_TYPE:
            ((RtSet*)data)->GCFlag = flag;
            return;
        case EXCEPTION_TYPE:
            ((RtException*)data)->GCFlag = flag;
            return;

    }

}

/**
 * DESCRIPTION:
 * Gets the GC Flag of pointer given the type
 * 
 * PARAMS:
 * data: data pointer
 * type: type of the pointer
*/
bool rttype_get_GCFlag(void *data, RtType type) {
    assert(data);
    switch (type)
    {
    case NULL_TYPE:
    case UNDEFINED_TYPE:
        return *((bool*)data);
    case NUMBER_TYPE:
        return ((RtNumber*)data)->GCFlag;
    case STRING_TYPE:
        return ((RtString*)data)->GCFlag;
    case LIST_TYPE:
        return ((RtList*)data)->GCFlag;
    case FUNCTION_TYPE:
        return ((RtFunction*)data)->GCFlag;
    case HASHMAP_TYPE:
        return ((RtMap*)data)->GCFlag;
    case CLASS_TYPE:
        return ((RtClass*)data)->GCFlag;
    case HASHSET_TYPE:
        return ((RtSet*)data)->GCFlag;
    case EXCEPTION_TYPE:
        return ((RtException*)data)->GCFlag;
    }
}

/**
 * DESCRIPTION:
 * Helper function for getting the refence count of some type
*/
size_t rttype_get_refcount(void* data, RtType type) {
    assert(data);
    switch (type)
    {
    case NULL_TYPE:
    case UNDEFINED_TYPE:
        return *((size_t*)data);
    case NUMBER_TYPE:
        return ((RtNumber*)data)->refcount;
    case STRING_TYPE:
        return ((RtString*)data)->refcount;
    case LIST_TYPE:
        return ((RtList*)data)->refcount;
    case FUNCTION_TYPE:
        return ((RtFunction*)data)->refcount;
    case HASHMAP_TYPE:
        return ((RtMap*)data)->refcount;
    case CLASS_TYPE:
        return ((RtClass*)data)->refcount;
    case HASHSET_TYPE:
        return ((RtSet*)data)->refcount;
    case EXCEPTION_TYPE:
        return ((RtException*)data)->refcount;
    }
}

/**
 * DESCRIPTION:
 * Helper for incrementing reference count
 * 
 * NOTE:
 * If function returns -1, then data pointer was NULL
*/
size_t rttype_increment_refcount(void* data, RtType type, size_t n) {
    if(!data)
        return -1;

    switch (type)
    {
    case NULL_TYPE:
    case UNDEFINED_TYPE:
        return *((size_t*)data) += n;
    case NUMBER_TYPE:
        return ((RtNumber*)data)->refcount += n;
    case STRING_TYPE:
        return ((RtString*)data)->refcount += n;
    case LIST_TYPE:
        return ((RtList*)data)->refcount += n;
    case FUNCTION_TYPE:
        return ((RtFunction*)data)->refcount += n;
    case HASHMAP_TYPE:
        return ((RtMap*)data)->refcount += n;
    case CLASS_TYPE:
        return ((RtClass*)data)->refcount += n;
    case HASHSET_TYPE:
        return ((RtSet*)data)->refcount += n;
    case EXCEPTION_TYPE:
        return ((RtException*)data)->refcount += n;
    }
}


/**
 * DESCRIPTION:
 * Helper for decrementing by n the reference count of some type
 * 
 * NOTE:
 * If function returns -1, then data pointer was NULL
*/
size_t rttype_decrement_refcount(void* data, RtType type, size_t n) {
    if(!data)
        return -1;
    
    size_t *refcount;

    switch (type)
    {
    case NULL_TYPE:
    case UNDEFINED_TYPE:
        refcount = ((size_t*)data);
        break;
    case NUMBER_TYPE:
        refcount = &((RtNumber*)data)->refcount;
        break;
    case STRING_TYPE:
        refcount = &((RtString*)data)->refcount;
        break;
    case LIST_TYPE:
        refcount = &((RtList*)data)->refcount;
        break;
    case FUNCTION_TYPE:
        refcount = &((RtFunction*)data)->refcount;
        break;
    case HASHMAP_TYPE:
        refcount = &((RtMap*)data)->refcount;
        break;
    case CLASS_TYPE:
        refcount = &((RtClass*)data)->refcount;
        break;
    case HASHSET_TYPE:
        refcount = &((RtSet*)data)->refcount;
        break;
    case EXCEPTION_TYPE:
        refcount = &((RtException*)data)->refcount;
        break;
    }
    assert((*refcount) >= n);

    *refcount -= n;

    return *refcount;
}






