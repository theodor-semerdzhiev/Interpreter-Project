#pragma once
#include "rtmap.h"
#include "../compiler/compiler.h"

typedef struct RtMap RtMap;

typedef struct RtClass {
    /// @brief These 2 fields are immutable, they DO NOT get freed during runtime
    char *classname;    
    RtFunction *body;
    
    //////

    RtMap *attrs_table;
} RtClass;

RtClass *init_RtClass(RtFunction *func, char *classname);