#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "rtattrs.h"
#include "builtins.h"
#include "rtattrslist.h"
#include "../runtime/rtobjects.h"
#include "../generics/utilities.h"
#include "../runtime/rtlists.h"
#include "../generics/hashmap.h"
#include "../runtime/rtobjects.h"

/**
 * DESCRIPTION:
 * This file will contain ALL built in (attribute) functions for all rt types
 */

static RtObject *builtin_map_add(RtObject *obj, RtObject **args, int argcount);

static GenericMap *attrsRegistry = NULL;

static const AttrBuiltinKey _map_add_key = {HASHMAP_TYPE, "add"};
static const AttrBuiltin _map_add =
    {HASHMAP_TYPE, {.builtin_func = builtin_map_add}, -1, "add", true};

static unsigned int _hash_attrbuiltin(const AttrBuiltinKey *attr)
{
    return djb2_string_hash(attr->attrname);
}

static bool _attrbuiltinin_equal(const AttrBuiltinKey *attr1, const AttrBuiltinKey *attr2)
{
    return strings_equal(attr1->attrname, attr2->attrname) &&
           attr1->target_type == attr2->target_type;
}

RtObject *rtattr_getattr(RtObject *obj, const char *attrname)
{
    AttrBuiltinKey key;
    key.attrname = attrname;
    key.target_type = obj->type;

    AttrBuiltin *val = (AttrBuiltin *)GenericHashMap_get(attrsRegistry, &key);
    if (!val)
    {
        assert(val);
        return NULL;
    }

    if (val->is_func)
    {
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
    else
    {
        RtObject *attr = val->func.get_attr(obj);
        assert(attr);
        return attr;
    }
}

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

    init_RtListAttr(attrsRegistry);

    addToAttrRegistry(attrsRegistry, _map_add_key, _map_add);
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
