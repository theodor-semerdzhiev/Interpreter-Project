#pragma once
#include "rttype.h"

typedef struct AttrBuiltin {
    RtType type;
    RtObject *(*builtin_func)(RtObject*, RtObject **, int);
    int argcount;

    char *attrsname;

} AttrBuiltin;

RtObject *rtattr_getfunc(RtObject *obj, const char* attrname);

void init_AttrRegistry();
void cleanup_AttrsRegistry();

RtObject *builtin_list_append(RtObject *list, RtObject **args, int argcount);