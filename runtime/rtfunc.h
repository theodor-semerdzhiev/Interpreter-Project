#pragma once
#include "rtobjects.h"

typedef struct ByteCodeList ByteCodeList;

typedef struct RtFunction
{
    bool is_builtin; // flag

    union
    {
        // regular user defined functions
        struct
        {
            ByteCodeList *body;

            // function arguments
            char **args;
            unsigned int arg_count;

            // each closures maps to the closure object
            char **closures;
            RtObject **closure_obj;

            unsigned int closure_count;

            char *func_name;
        } user_func;

        // built in function
        struct built_in
        {
            Builtin *func;
        } built_in;
    } func_data;

    bool GCFlag; // used by GC for garbage collection
} RtFunction;

RtObject *mutate_func_data(RtObject *target, const RtObject *new_val, bool deepcpy);
void free_func_data(RtObject *obj, bool free_immutable);
void rtfunc_free(RtFunction *func, bool free_immutable);
char *rtfunc_toString(RtFunction *function);
RtFunction *init_rtfunc(bool builtin);
RtFunction *rtfunc_cpy(const RtFunction *func, bool deepcpy);
RtObject **rtfunc_getrefs(const RtFunction *func);