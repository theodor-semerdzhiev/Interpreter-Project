#pragma once
#include "rtmap.h"
#include "../compiler/compiler.h"

typedef struct RtMap RtMap;

typedef struct RtClass {
    /// @brief These 2 fields are immutable, they DO NOT get freed during runtime
    char *classname;    
    RtFunction *body;
    
    RtMap *attrs_table;
    size_t refcount;
} RtClass;

RtClass *init_RtClass(char *classname);


void rtclass_free(RtClass *class, bool free_refs, bool free_immutable, bool update_ref_counts);
char *rtclass_toString(const RtClass *cls);
RtClass *rtclass_cpy(const RtClass *class, bool deepcpy, bool add_to_GC);
