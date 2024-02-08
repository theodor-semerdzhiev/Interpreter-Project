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

static RtObject *builtin_list_append(RtObject *list, RtObject **args, int argcount);
static RtObject *builtin_list_pop(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_list_popLast(RtObject *obj, RtObject **args, int argcount);

static RtObject *builtin_map_add(RtObject *obj, RtObject **args, int argcount);

typedef struct AttrBuiltinKey
{
    RtType type;
    const char *attrname;
} AttrBuiltinKey;

static GenericMap *attrsRegistry = NULL;

static const AttrBuiltinKey _list_append_key = {LIST_TYPE, "append"};
static const AttrBuiltin _list_append = {LIST_TYPE, builtin_list_append, -1, "append"};

static const AttrBuiltinKey _list_pop_key = {LIST_TYPE, "pop"};
static const AttrBuiltin _list_pop = {LIST_TYPE, builtin_list_pop, -1, "pop"};

static const AttrBuiltinKey _list_popLast_key = {LIST_TYPE, "popLast"};
static const AttrBuiltin _list_popLast = {LIST_TYPE, builtin_list_popLast, -1, "popLast"};

static const AttrBuiltinKey _map_add_key = {HASHMAP_TYPE, "add"};
static const AttrBuiltin _map_add = {HASHMAP_TYPE, builtin_map_add, -1, "add"};

static unsigned int _hash_attrbuiltin(const AttrBuiltinKey *attr)
{
    return djb2_string_hash(attr->attrname);
}

static bool _attrbuiltinin_equal(const AttrBuiltinKey *attr1, const AttrBuiltinKey *attr2)
{
    return strings_equal(attr1->attrname, attr2->attrname) &&
           attr1->type == attr2->type;
}

RtObject *rtattr_getfunc(RtObject *obj, const char *attrname)
{
    AttrBuiltinKey key;
    key.attrname = attrname;
    key.type = obj->type;

    AttrBuiltin *val = (AttrBuiltin *)GenericHashMap_get(attrsRegistry, &key);
    if (!val)
    {
        return NULL;
    }

    RtFunction *attr = init_rtfunc(ATTR_BUILTIN);
    if (!attr)
        MallocError();
    attr->func_data.attr_built_in.func = val;
    attr->func_data.attr_built_in.target = obj;

    RtObject *func = init_RtObject(FUNCTION_TYPE);
    if (!func)
        MallocError();
    func->data.Func = attr;

    return func;
}

#define addToAttrRegistry(key, val) GenericHashMap_insert(attrsRegistry, (void*)&key, (void*)&val, false)

void init_AttrRegistry()
{
    if (attrsRegistry)
        return;

    attrsRegistry = init_GenericMap(
        (unsigned int (*)(const void *))_hash_attrbuiltin,
        (bool (*)(const void *, const void *))_attrbuiltinin_equal,
        NULL,
        NULL);

    if (!attrsRegistry)
    {
        MallocError();
    }

    addToAttrRegistry(_list_append_key, _list_append);
    addToAttrRegistry(_list_pop_key, _list_pop);
    addToAttrRegistry(_list_popLast_key, _list_popLast);

    addToAttrRegistry(_map_add_key, _map_add);
}

void cleanup_AttrsRegistry()
{
    if (!attrsRegistry)
        return;
    free_GenericMap(attrsRegistry, false, false);
    attrsRegistry = NULL;
}

/**
 * DESCRIPTION:
 * Builtin function for adding objects to a rt list
 */
static RtObject *builtin_list_append(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount > 0); // temporary
    assert(obj->type == LIST_TYPE);
    RtList *list = obj->data.List;
    for (int i = 0; i < argcount; i++)
    {
        rtlist_append(list, args[i]);
    }
    return obj;
}

/**
 * DESCRIPTION:
 * Builtin function for removing objects from a rt list, at a specific index
 */
static RtObject *builtin_list_pop(RtObject *obj, RtObject **args, int argcount)
{
    // temporary
    assert(argcount  == 1); 
    assert(obj->type == LIST_TYPE);
    assert(args[0]->type == NUMBER_TYPE);

    RtList *list = obj->data.List;
    double number = args[0]->data.Number->number;

    rtlist_remove(list, (size_t)number);
    return obj;
}

/**
 * DESCRIPTION:
 * Built in function for adding a element into a specific index of a rt list
*/
static RtObject *builtin_list_popLast(RtObject *obj, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 0); 
    assert(obj->type == LIST_TYPE);
    rtlist_poplast(obj->data.List);
    return obj;
}

/**
 * DESCRIPTION:
 * Builtin function for adding key value pairs to a map
 */
static RtObject *builtin_map_add(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount == 2); // temporary
    assert(obj->type == HASHMAP_TYPE);

    RtMap *map = obj->data.Map;
    RtObject *key = args[0];
    RtObject *val = args[1];
    rtmap_insert(map, key, val);
    return obj;
}
