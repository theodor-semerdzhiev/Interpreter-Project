#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "builtinexception.h"
#include "builtinfuncs.h"
#include "../generics/hashmap.h"
#include "../generics/utilities.h"


#define InsertBuiltIn(name, builtin_struct) GenericHashMap_insert(BuiltinFunc_Registry, name, (void *)&builtin_struct, false)

static RtObject *builtin_Exception(RtObject **args, int arg_count);

static const BuiltinFunc _builtin_print = {"Exception", builtin_Exception, 1};

/**
 * DESCRIPTION:
 * Initializes all built in exception and puts them into the builtin function registry
 * returns 1 -> initialization was successful
 * return 0 -> initialization failed
*/
int init_BuiltinException(GenericMap *BuiltinFunc_Registry) {
    assert(BuiltinFunc_Registry);

    return 1;
}


static RtObject *builtin_Exception(RtObject **args, int arg_count) {
    //temporary
    assert(arg_count > 1);
    // TODO
    return NULL;
}



