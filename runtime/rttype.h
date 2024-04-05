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
size_t rttype_get_refcount(void* data, RtType type);


