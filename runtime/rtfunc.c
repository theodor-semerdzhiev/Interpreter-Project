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
RtObject *mutate_func_data(RtObject *target, const RtObject *new_val, bool deepcpy, bool add_to_GC)
{
    assert(target->type == FUNCTION_TYPE && new_val->type == FUNCTION_TYPE && new_val->data.Func);
    target->data.Func = rtfunc_cpy(new_val->data.Func, deepcpy);
    if (add_to_GC)
        add_to_GC_registry(target);
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
init_rtfunc(RtFuncType type)
{
    RtFunction *func = malloc(sizeof(RtFunction));
    if (!func)
        return NULL;
    func->functype = type;
    func->refcount = 0;
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
void rtfunc_free(RtFunction *func, bool free_immutable, bool update_ref_counts)
{
    if (!func)
        return;

    if (func->functype == EXCEPTION_CONSTRUCTOR_FUNC)
    {
        free(func->func_data.exception_constructor.exception_name);
        free(func);
        return;
    }

    if (func->functype == BUILTIN_FUNC)
    {
        free(func);
        return;

    }
    if(func->functype == ATTR_BUILTIN_FUNC) {
        // updates reference count
        if(update_ref_counts)
            rtobj_refcount_decrement1(func->func_data.attr_built_in.target);
        free(func);
        return;
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
        free(func->func_data.user_func.file_location);
        free_ByteCodeList(func->func_data.user_func.body);
    }
    else
    {

        // closure objects are dynamic (determined during runtime), so they are freed
        // updates reference count
        if(update_ref_counts) {
            for (size_t i=0; i < func->func_data.user_func.closure_count; i++)
                rtobj_refcount_decrement1(func->func_data.user_func.closure_obj[i]);
        }

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
void free_func_data(RtObject *obj, bool free_immutable, bool update_ref_counts)
{
    assert(obj->type == FUNCTION_TYPE && obj->data.Func);
    rtfunc_free(obj->data.Func, free_immutable, update_ref_counts);
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
    RtFunction *cpy = init_rtfunc(func->functype);
    if (!cpy)
        return NULL;

    switch (func->functype)
    {
    case EXCEPTION_CONSTRUCTOR_FUNC:
    {
        cpy->func_data.exception_constructor.exception_name =
            cpy_string(func->func_data.exception_constructor.exception_name);
        break;
    }
    case BUILTIN_FUNC:
    {
        cpy->func_data.built_in.func = func->func_data.built_in.func;
        break;
    }

    case ATTR_BUILTIN_FUNC:
    {
        cpy->func_data.attr_built_in.func = func->func_data.attr_built_in.func;
        cpy->func_data.attr_built_in.target = func->func_data.attr_built_in.target;
        rtobj_refcount_increment1(cpy->func_data.attr_built_in.target);
        break;
    }

    case REGULAR_FUNC:
    {
        cpy->func_data.user_func.arg_count = func->func_data.user_func.arg_count;
        cpy->func_data.user_func.args = func->func_data.user_func.args;
        cpy->func_data.user_func.body = func->func_data.user_func.body;
        cpy->func_data.user_func.closure_count = func->func_data.user_func.closure_count;
        cpy->func_data.user_func.closures = func->func_data.user_func.closures;
        cpy->func_data.user_func.func_name = func->func_data.user_func.func_name;
        cpy->func_data.user_func.file_location = func->func_data.user_func.file_location;

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
            {
                new_arr[i] = func->func_data.user_func.closure_obj[i];
                rtobj_refcount_increment1(new_arr[i]);
            }

            cpy->func_data.user_func.closure_obj = new_arr;
        }
        break;
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
    assert(function);
    switch (function->functype)
    {
    case EXCEPTION_CONSTRUCTOR_FUNC:
    {
        char *exception_name = function->func_data.exception_constructor.exception_name;
        size_t buffer_length = 64 + strlen(exception_name);
        char buffer[buffer_length];
        snprintf(buffer, sizeof(buffer), "%s.exception_constructor_func@%p", exception_name, function);
        return cpy_string(buffer);
    }
    case BUILTIN_FUNC:
    {
        BuiltinFunc *func = function->func_data.built_in.func;
        size_t buffer_length = 65 + strlen(func->builtin_name) + 2 * sizeof(void *) + 1;
        char buffer[buffer_length];
        snprintf(buffer, sizeof(buffer), "%s.builtin_func@%p", func->builtin_name, function);
        return cpy_string(buffer);
    }

    case ATTR_BUILTIN_FUNC:
    {
        AttrBuiltin *func = function->func_data.attr_built_in.func;
        size_t buffer_length = strlen(func->attrsname) + 100;
        char buffer[buffer_length];
        snprintf(buffer, sizeof(buffer), "%s.%s.attrbuiltin_func@%p",
                 func->attrsname,
                 rtobj_type_toString(function->func_data.attr_built_in.target->type),
                 function);

        return cpy_string(buffer);
    }

    case REGULAR_FUNC:
    {

        if (function->func_data.user_func.func_name)
        {
            char *funcname = function->func_data.user_func.func_name;
            size_t buffer_length = 65 + strlen(funcname) + 2 * sizeof(void *) + 1;
            char buffer[buffer_length];
            snprintf(buffer, sizeof(buffer), "%s.func@%p", funcname, function->func_data.user_func.body);
            return cpy_string(buffer);
        }
        // for unnamed functions (i.e inline functions)
        else
        {
            size_t buffer_length = 80 + 2 * sizeof(void *) + 1;
            char buffer[buffer_length];
            snprintf(buffer, sizeof(buffer), "(unknown).func@%p", function->func_data.user_func.body);

            return cpy_string(buffer);
        }
    }
    }
    return NULL;
}

/**
 * DESCRIPTION:
 * Prints out rt function
 */
void rtfunc_print(RtFunction *func)
{
    assert(func);
    char *str = rtfunc_toString(func);
    printf("%s", str);
    free(str);
}

/**
 * DESCRIPTION:
 * Hashes a RtFunc struct
 */
unsigned int rtfunc_hash(const RtFunction *func)
{
    switch (func->functype)
    {
    case EXCEPTION_CONSTRUCTOR_FUNC:
        return hash_pointer((void *)func->func_data.exception_constructor.exception_name);

    case REGULAR_FUNC:
        return hash_pointer((void *)func->func_data.user_func.body);

    case BUILTIN_FUNC:
        return hash_pointer((void *)func->func_data.built_in.func);

    case ATTR_BUILTIN_FUNC:
        return hash_pointer((void *)func->func_data.attr_built_in.func);
    }
}

/**
 * DESCRIPTION:
 * Logic for checking if 2 rt functions struct are equal
 */
bool rtfunc_equal(const RtFunction *func1, const RtFunction *func2)
{
    if (func1 == func2)
        return true;

    if (func1->functype != func2->functype)
        return false;

    switch (func1->functype)
    {
    case EXCEPTION_CONSTRUCTOR_FUNC:
    {
        char *e1 = func1->func_data.exception_constructor.exception_name;
        char *e2 = func2->func_data.exception_constructor.exception_name;
        return strcmp(e1, e2) ? 1 : 0;
    }
    case REGULAR_FUNC:
        return func1->func_data.user_func.body == func2->func_data.user_func.body;
    case BUILTIN_FUNC:
        return func1->func_data.built_in.func == func2->func_data.built_in.func;
    case ATTR_BUILTIN_FUNC:
        return func1->func_data.attr_built_in.func == func2->func_data.attr_built_in.func;
    }

    return false;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Creates a NULL terminated array of all objects that the function is referencing (i.e still binded too)
 */
RtObject **
rtfunc_getrefs(const RtFunction *func)
{
    if (func->functype == ATTR_BUILTIN_FUNC)
    {
        RtObject **refs = malloc(sizeof(RtObject *) * 2);
        if (!refs)
            MallocError();
        refs[1] = NULL;
        refs[0] = func->func_data.attr_built_in.target;
        return refs;
    }

    if (func->functype == BUILTIN_FUNC && func->functype == EXCEPTION_CONSTRUCTOR_FUNC)
    {
        RtObject **refs = malloc(sizeof(RtObject *));
        if (!refs)
            MallocError();
        refs[0] = NULL;
        return refs;
    }

    unsigned int length = func->func_data.user_func.closure_count;
    RtObject **refs = malloc(sizeof(RtObject *) * (length + 1));
    if (!refs)
        MallocError();
    for (unsigned int i = 0; i < length; i++)
    {
        refs[i] = func->func_data.user_func.closure_obj[i];
    }
    refs[length] = NULL;
    return refs;
}

/**
 * DESCRIPTION:
 * Returns the type (in string format) of the type of the input function
 */
const char *rtfunc_type_toString(const RtFunction *func)
{
    switch (func->functype)
    {
    case BUILTIN_FUNC:
        return "Builtin";
    case ATTR_BUILTIN_FUNC:
        return "Attribute Builtin";
    case REGULAR_FUNC:
        return "Regular";
    case EXCEPTION_CONSTRUCTOR_FUNC:
        return "Exception Constructor";
    }
}

/**
 * DESCRIPTION:
 * Gets the name of the function, function returns the direct reference, it does not create a copy
 */
const char *rtfunc_get_funcname(const RtFunction *func)
{
    switch (func->functype)
    {
    case REGULAR_FUNC:
    {
        return func->func_data.user_func.func_name ? func->func_data.user_func.func_name : "(unknown)";
    }
    case EXCEPTION_CONSTRUCTOR_FUNC:
    {
        return func->func_data.exception_constructor.exception_name;
    }
    case BUILTIN_FUNC:
    {
        return func->func_data.built_in.func->builtin_name;
    }
    case ATTR_BUILTIN_FUNC:
    {
        return func->func_data.attr_built_in.func->attrsname;
    }
    }
}
