#pragma once
#include "rtobjects.h"
#include "../rtlib/rtattrs.h"

typedef struct ByteCodeList ByteCodeList;
typedef struct AttrBuiltin AttrBuiltin;

typedef enum RtFuncType {
    REGULAR,
    REGULAR_BUILTIN,
    ATTR_BUILTIN
} RtFuncType;

typedef struct RtFunction
{
    // bool is_builtin; // flag
    RtFuncType functype;

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
        struct
        {
            Builtin *func; // IMMUTABLE
        } built_in;

        struct {
            AttrBuiltin *func; // IMMUTABLE 
            RtObject *target; 
        } attr_built_in;
    } func_data;

    bool GCFlag; // used by GC for garbage collection
} RtFunction;

RtObject *mutate_func_data(RtObject *target, const RtObject *new_val, bool deepcpy);
void free_func_data(RtObject *obj, bool free_immutable);
void rtfunc_free(RtFunction *func, bool free_immutable);
char *rtfunc_toString(RtFunction *function);
unsigned int rtfunc_hash(const RtFunction *func);
bool rtfunc_equal(const RtFunction *func1, const RtFunction *func2);
RtFunction *init_rtfunc(RtFuncType type);
RtFunction *rtfunc_cpy(const RtFunction *func, bool deepcpy);
RtObject **rtfunc_getrefs(const RtFunction *func);
const char* rtfunc_type_toString(const RtFunction *func);
void rtfunc_print(RtFunction *func);