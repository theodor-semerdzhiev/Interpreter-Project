#pragma once
#include "rtobjects.h"
#include "../rtlib/rtattrs.h"

typedef struct ByteCodeList ByteCodeList;
typedef struct AttrBuiltin AttrBuiltin;

typedef enum RtFuncType
{
    REGULAR_FUNC,
    BUILTIN_FUNC,
    ATTR_BUILTIN_FUNC,
    EXCEPTION_CONSTRUCTOR_FUNC
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
            size_t arg_count;

            // each closures maps to the closure object
            char **closures;
            RtObject **closure_obj;

            size_t closure_count;

            char *func_name;

            char* file_location; // name of the file where this function is declared
        } user_func;

        // built in function
        struct
        {
            BuiltinFunc *func; // IMMUTABLE
        } built_in;

        // built in attribute function
        struct
        {
            AttrBuiltin *func; // IMMUTABLE
            RtObject *target;
        } attr_built_in;

        // Created during runtime 
        struct
        {
            char *exception_name;
        } exception_constructor;
    } func_data;

    size_t refcount; // reference count
} RtFunction;

RtObject *mutate_func_data(RtObject *target, const RtObject *new_val, bool deepcpy, bool add_to_GC);
void free_func_data(RtObject *obj, bool free_immutable, bool update_ref_counts);
void rtfunc_free(RtFunction *func, bool free_immutable, bool update_ref_counts);
char *rtfunc_toString(RtFunction *function);
unsigned int rtfunc_hash(const RtFunction *func);
bool rtfunc_equal(const RtFunction *func1, const RtFunction *func2);
RtFunction *init_rtfunc(RtFuncType type);
RtFunction *rtfunc_cpy(const RtFunction *func, bool deepcpy);
RtObject **rtfunc_getrefs(const RtFunction *func);
const char *rtfunc_type_toString(const RtFunction *func);
const char *rtfunc_get_funcname(const RtFunction *func);
void rtfunc_print(RtFunction *func);