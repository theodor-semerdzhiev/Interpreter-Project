#pragma once
#include <stdbool.h>
#include <stddef.h>

// Possible runtime types
typedef enum RtType
{
    UNDEFINED_TYPE,
    NULL_TYPE,
    NUMBER_TYPE,
    STRING_TYPE,
    CLASS_TYPE,
    FUNCTION_TYPE,
    LIST_TYPE,
    HASHMAP_TYPE,
    HASHSET_TYPE,
    EXCEPTION_TYPE
} RtType;

#define NB_OF_TYPES 10

bool rttype_isprimitive(RtType type);
const char *rtobj_type_toString(RtType type);
void rttype_freedata(RtType type, void *data, bool freerefs, bool update_ref_counts);   
void rttype_set_GCFlag(void* data, RtType type, bool flag);
bool rttype_get_GCFlag(void *data, RtType type);
size_t rttype_get_refcount(void* data, RtType type);
size_t rttype_increment_refcount(void* data, RtType type, size_t n);
size_t rttype_decrement_refcount(void* data, RtType type, size_t n);

/* A few macro */
#define rtobj_refcount(obj) rttype_get_refcount(rtobj_getdata(obj), obj->type)
#define rtobj_refcount_increment1(obj) rttype_increment_refcount(rtobj_getdata(obj), obj->type, 1)
#define rtobj_refcount_decrement1(obj) rttype_decrement_refcount(rtobj_getdata(obj), obj->type, 1)
