#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "rtobjects.h"
#include "../compiler/compiler.h"
#include "../generics/utilities.h"
#include "gc.h"

/**
 * DESCRIPTION:
 * Function runtime type implementation
 */

/**
 * DESCRIPTION:
 * Logic for mutating target object, will always return target parameter
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
    assert(target->type == FUNCTION_TYPE && new_val->type == FUNCTION_TYPE);

    // performs shallow copy every time
    target->data.Function.is_builtin = new_val->data.Function.is_builtin;

    if (new_val->data.Function.is_builtin)
    {
        target->data.Function.func_data.built_in.func =
            new_val->data.Function.func_data.built_in.func;
    }
    else
    {
        target->data.Function.func_data.user_func.func_name =
            new_val->data.Function.func_data.user_func.func_name;

        target->data.Function.func_data.user_func.args =
            new_val->data.Function.func_data.user_func.args;

        target->data.Function.func_data.user_func.closures =
            new_val->data.Function.func_data.user_func.closures;

        if (!deepcpy)
        {
            target->data.Function.func_data.user_func.closure_obj =
                new_val->data.Function.func_data.user_func.closure_obj;
        }
        else if (new_val->data.Function.func_data.user_func.closure_obj)
        {
            // since closure objects are determined during runtime
            // a new array is allocated
            target->data.Function.func_data.user_func.closure_obj =
                malloc(sizeof(RtObject *) * new_val->data.Function.func_data.user_func.closure_count);
            for (int i = 0; i < new_val->data.Function.func_data.user_func.closure_count; i++)
                target->data.Function.func_data.user_func.closure_obj[i] =
                    new_val->data.Function.func_data.user_func.closure_obj[i];
        }
        else
        {
            target->data.Function.func_data.user_func.closure_obj = NULL;
        }

        target->data.Function.func_data.user_func.closure_count =
            new_val->data.Function.func_data.user_func.closure_count;

        target->data.Function.func_data.user_func.arg_count =
            new_val->data.Function.func_data.user_func.arg_count;

        target->data.Function.func_data.user_func.body =
            new_val->data.Function.func_data.user_func.body;
    }
    return target;
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
void free_func_data(RtObject *obj, bool free_immutable) {
    if (free_immutable && !obj->data.Function.is_builtin) {
        for (int i = 0; i < obj->data.Function.func_data.user_func.arg_count; i++)
        {
            free(obj->data.Function.func_data.user_func.args[i]);
        }
        for (int i = 0; i < obj->data.Function.func_data.user_func.closure_count; i++)
        {
            free(obj->data.Function.func_data.user_func.closures[i]);
        }
        free(obj->data.Function.func_data.user_func.closures);
        free(obj->data.Function.func_data.user_func.args);
        free(obj->data.Function.func_data.user_func.closure_obj);
        free(obj->data.Function.func_data.user_func.func_name);
        free_ByteCodeList(obj->data.Function.func_data.user_func.body);
    }

    if (!obj->data.Function.is_builtin)
        // TODO: This causes a really weird double free error for test1.txt
        free(obj->data.Function.func_data.user_func.closure_obj);
}
