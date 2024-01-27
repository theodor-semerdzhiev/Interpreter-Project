#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "rtobjects.h"
#include "../compiler/compiler.h"
#include "../generics/utilities.h"
#include "gc.h"
#include "rtfunc.h"

/**
 * DESCRIPTION:
 * Function runtime type implementation
 */

/**
 * DESCRIPTION:
 * Logic for mutating target object, will always return target parameter. All it those is create a copy of the RtFunction object
 *
 * PRECONDITION:
 * Both runtime objects must be function types
 *
 * PARAMS:
 * target: object to be mutated
 * func: object to cpy from
 * deepcpy: wether we should do a deep copy
 *
 */
RtObject *mutate_func_data(RtObject *target, const RtObject *new_val, bool deepcpy)
{
    assert(target->type == FUNCTION_TYPE && new_val->type == FUNCTION_TYPE && new_val->data.Func);
    target->data.Func = rtfunc_cpy(new_val->data.Func, deepcpy);
    return target;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Mallocs function struct, return NULL if malloc fails
 *
 * PARAMS:
 * builtin: wether function represents a built in function (or not)
 */
RtFunction *
init_rtfunc(bool builtin)
{
    RtFunction *func = malloc(sizeof(RtFunction));
    if (!func)
        return NULL;
    func->is_builtin = builtin;
    func->GCFlag = false;
    return func;
}

/**
 * DESCRIPTION:
 * Frees RtFunction struct
 *
 * PARAMS:
 * func: address to free
 * free_immutable: wether function data bytecode (and other associated data should be freed)
 */
void rtfunc_free(RtFunction *func, bool free_immutable)
{
    if (!func)
        return;
    if (func->is_builtin)
    {
        free(func);
        return;

        // data being freed here is immutable, its never freed and exists only in the raw bytecode
    }

    if (free_immutable)
    {
        for (unsigned int i = 0; i < func->func_data.user_func.arg_count; i++)
        {
            free(func->func_data.user_func.args[i]);
        }
        for (unsigned int i = 0; i < func->func_data.user_func.closure_count; i++)
        {
            free(func->func_data.user_func.closures[i]);
        }
        free(func->func_data.user_func.closures);
        free(func->func_data.user_func.args);
        free(func->func_data.user_func.closure_obj);
        free(func->func_data.user_func.func_name);
        free_ByteCodeList(func->func_data.user_func.body);
    }
    else
    {
        // closure objects are dynamic (determined during runtime), so they are freed
        free(func->func_data.user_func.closure_obj);
    }
    free(func);
}

/**
 * DESCRIPTION:
 * Frees function object data.
 * NOTE: Builtin function data WILL never be freed, since it lives on its own
 *
 * PRECONDITION:
 * input object must be a function type
 *
 * PARAMS:
 * obj: object to free
 * free_immutable: wether function data bytecode (and other associated data should be freed)
 */
void free_func_data(RtObject *obj, bool free_immutable)
{
    assert(obj->type == FUNCTION_TYPE && obj->data.Func);
    rtfunc_free(obj->data.Func, free_immutable);
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Performs a shallow or deepcpy of a RtFunction. In practice, a built in function is always shallowed copy since only one instance exists at any given time
 *
 * PARAMS:
 * func: function to copy
 * deepcpy: wether a deep or shallow copy should be performed
 *
 * NOTE:
 * This function will return NULL if malloc fails
 */
RtFunction *
rtfunc_cpy(const RtFunction *func, bool deepcpy)
{
    RtFunction *cpy = init_rtfunc(func->is_builtin);
    if (!cpy)
        return NULL;

    if (func->is_builtin)
    {
        cpy->func_data.built_in.func = func->func_data.built_in.func;
    }
    else
    {
        cpy->func_data.user_func.arg_count = func->func_data.user_func.arg_count;
        cpy->func_data.user_func.args = func->func_data.user_func.args;
        cpy->func_data.user_func.body = func->func_data.user_func.body;
        cpy->func_data.user_func.closure_count = func->func_data.user_func.closure_count;
        cpy->func_data.user_func.closures = func->func_data.user_func.closures;
        cpy->func_data.user_func.func_name = func->func_data.user_func.func_name;

        // value of this can vary during runtime
        // special case
        if (!deepcpy || func->func_data.user_func.closure_count == 0 || !func->func_data.user_func.closure_obj)
        {
            cpy->func_data.user_func.closure_obj = NULL;
        }
        else
        {
            RtObject **new_arr = malloc(sizeof(RtObject *) * func->func_data.user_func.closure_count);
            if (!new_arr)
            {
                free(cpy);
                return NULL;
            }
            for (unsigned int i = 0; i < func->func_data.user_func.closure_count; i++)
                new_arr[i] = func->func_data.user_func.closure_obj[i];

            cpy->func_data.user_func.closure_obj = new_arr;
        }
    }

    return cpy;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Returns a sring representation of a function object
 */
char *
rtfunc_toString(RtFunction *function)
{
    // for built in functions
    if (function->is_builtin)
    {
        Builtin *func = function->func_data.built_in.func;
        size_t buffer_length = 65 + strlen(func->builtin_name) + 2 * sizeof(void *) + 1;
        char buffer[buffer_length];
        snprintf(buffer, sizeof(buffer), "%s.func@%p", func->builtin_name, func->builtin_func);
        return cpy_string(buffer);
    }
    // for named functions
    else if (function->func_data.user_func.func_name)
    {
        char *funcname = function->func_data.user_func.func_name;
        size_t buffer_length = 65 + strlen(funcname) + 2 * sizeof(void *) + 1;
        char buffer[buffer_length];
        snprintf(buffer, sizeof(buffer), "%s.func@%p", funcname, function->func_data.built_in.func);
        return cpy_string(buffer);
    }
    // for unnamed functions (i.e inline functions)
    else
    {
        size_t buffer_length = 80 + 2 * sizeof(void *) + 1;
        char buffer[buffer_length];
        snprintf(buffer, sizeof(buffer), "func@%p", function->func_data.user_func.func_name);

        return cpy_string(buffer);
    }
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Creates a NULL terminated array of all objects that the function is referencing (i.e still binded too)
 */
RtObject **
rtfunc_getrefs(const RtFunction *func)
{
    if(func->is_builtin) {
        RtObject **refs = malloc(sizeof(RtObject *));
        if(!refs) return NULL;
        refs[0]=NULL;
        return refs;
    }
    unsigned int length = func->func_data.user_func.closure_count;
    RtObject **refs = malloc(sizeof(RtObject *) * (length + 1));
    if (!refs)
        return NULL;
    for (unsigned int i = 0; i < length; i++)
    {
        refs[i] = func->func_data.user_func.closure_obj[i];
    }
    refs[length] = NULL;
    return refs;
}
