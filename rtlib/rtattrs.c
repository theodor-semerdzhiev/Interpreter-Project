#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "rtattrs.h"
#include "builtins.h"
#include "rtattrslist.h"
#include "rtattrsmap.h"
#include "rtattrsset.h"
#include "../runtime/rtobjects.h"
#include "../generics/utilities.h"
#include "../runtime/rtlists.h"
#include "../generics/hashmap.h"
#include "../runtime/rtobjects.h"

/**
 * DESCRIPTION:
 * This file will contain ALL built in (attribute) functions for all rt types
 */

static GenericMap *attrsRegistry = NULL;

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
    init_RtMapAttr(attrsRegistry);
    init_RtSetAttr(attrsRegistry);
}

void cleanup_AttrsRegistry()
{
    if (!attrsRegistry)
        return;
    free_GenericMap(attrsRegistry, false, false);
    attrsRegistry = NULL;
}

