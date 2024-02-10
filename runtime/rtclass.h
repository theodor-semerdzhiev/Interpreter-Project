#pragma once
#include "rtmap.h"
#include "../compiler/compiler.h"

typedef struct RtSet RtSet;

typedef struct RtClass {
    /// @brief These 2 fields are immutable, they DO NOT get freed during runtime
    char *classname;    
    RtFunction *body;
    
    //////

    RtSet *attrs_table;

    bool GCFlag;
} RtClass;

RtClass *init_RtClass(char *classname);


void rtclass_free(RtClass *class, bool free_refs, bool free_immutable);
char *rtclass_toString(const RtClass *cls);
RtClass *rtclass_cpy(const RtClass *class, bool deepcpy, bool add_to_GC);
