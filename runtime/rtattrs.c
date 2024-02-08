#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "rtobjects.h"
#include "builtins.h"
#include "../generics/utilities.h"
#include "rtattrs.h"
#include "rtlists.h"
#include "../generics/hashmap.h"

/**
 * DESCRIPTION:
 * This file will contain ALL built in (attribute) functions for all rt types
*/



typedef struct AttrBuiltinKey {
    RtType type;
    const char *attrname;
} AttrBuiltinKey;

static GenericMap *attrsRegistry = NULL;

static const AttrBuiltinKey _list_append_key = {LIST_TYPE, "append"};
static const AttrBuiltin _list_append = {LIST_TYPE, builtin_list_append, -1, "append" };

static unsigned int _hash_attrbuiltin(const AttrBuiltinKey *attr) {
    return djb2_string_hash(attr->attrname);
}

static bool _attrbuiltinin_equal(const AttrBuiltinKey *attr1, const AttrBuiltinKey *attr2) {
    return 
    strings_equal(attr1->attrname, attr2->attrname) && 
    attr1->type == attr2->type;
}

RtObject *rtattr_getfunc(RtObject *obj, const char* attrname) {
    AttrBuiltinKey key;
    key.attrname = attrname;
    key.type = obj->type;

    AttrBuiltin *val = (AttrBuiltin*)GenericHashMap_get(attrsRegistry, &key);
    if(!val) {
        return NULL;
    }

    RtFunction *attr = init_rtfunc(ATTR_BUILTIN);
    if(!attr) MallocError();
    attr->func_data.attr_built_in.func=val;
    attr->func_data.attr_built_in.target=obj;

    RtObject *func = init_RtObject(FUNCTION_TYPE);
    if(!func) MallocError();
    func->data.Func=attr;

    return func;
}

void init_AttrRegistry() {
    if(attrsRegistry) return;

    attrsRegistry = init_GenericMap(
        (unsigned int (*)(const void *))_hash_attrbuiltin,
        (bool (*)(const void *, const void *))_attrbuiltinin_equal, 
        NULL,
        NULL
    );

    if(!attrsRegistry) {
        MallocError();
    }

    GenericHashMap_insert(attrsRegistry, (void*)&_list_append_key, (void*)&_list_append, false);
}

void cleanup_AttrsRegistry() {
    if(!attrsRegistry) return;
    free_GenericMap(attrsRegistry, false, false);
    attrsRegistry = NULL;
}

/**
 * DESCRIPTION:
 * Builtin function for adding objects to a rt list
*/
RtObject *builtin_list_append(RtObject *obj, RtObject **args, int argcount) {
    assert(argcount > 0); // temporary
    assert(obj->type == LIST_TYPE);
    RtList *list = obj->data.List;
    for(int i=0; i< argcount; i++) {
        rtlist_append(list, args[i]);
    }
    return obj;
}

