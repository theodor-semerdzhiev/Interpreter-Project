#pragma once
#include "../runtime/rttype.h"
#include "../runtime/rtobjects.h"

typedef struct AttrBuiltin
{
    RtType target_type;

    union
    {
        RtObject *(*builtin_func)(RtObject *, RtObject **, int);
        RtObject *(*get_attr)(RtObject *);
    } func;

    int argcount;
    char *attrsname;

    bool is_func; // wether attribute is a builtin function or a field
} AttrBuiltin;

typedef struct AttrBuiltinKey
{
    RtType target_type;
    const char *attrname;
} AttrBuiltinKey;

#define addToAttrRegistry(reg, key, val) GenericHashMap_insert(reg, (void *)&key, (void *)&val, false)

RtObject *rtattr_getattr(RtObject *obj, const char *attrname);

void init_AttrRegistry();
void cleanup_AttrsRegistry();
