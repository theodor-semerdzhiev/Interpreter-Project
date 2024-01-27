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
*/
void rttype_freedata(RtType type, void *data, bool freerefs) {
    switch (type)
    {
    case NULL_TYPE:
    case UNDEFINED_TYPE:
    case NUMBER_TYPE:
        return;
    
    case STRING_TYPE:
        rtstr_free(data);
        return;

    case LIST_TYPE:
        rtlist_free((RtList*)data);
        return;
    
    case FUNCTION_TYPE: 
        rtfunc_free((RtFunction*) data, false);
        return;
    case HASHMAP_TYPE:
        rtmap_free((RtMap*)data, freerefs, freerefs, false);
        return;

    case CLASS_TYPE:
        rtclass_free((RtClass*)data, freerefs, false);
        return;
    
    }
}


