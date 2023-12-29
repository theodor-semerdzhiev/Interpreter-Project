#include "compiler.h"
#include "runtime.h"
#include "rtobjects.h"
#include "generics/utilities.h"
#include "generics/hashmap.h"
#include "builtins.h"
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * This file contains the implementation of all general built in functions:
 * - print
 *
 */

static GenericMap *builtin_map = NULL;

/* Print function */
static const Builtin _builtin_print = {"print", builtin_print, -1};
static const Builtin _builtin_println = {"println", builtin_println, -1};

/**
 * Defines equality for built in functions
 */
static bool builtins_equal(const Builtin *builtin1, const Builtin *builtin2)
{
    return builtin1->arg_count == builtin2->arg_count && builtin1->builtin_func == builtin2->builtin_func;
}

/**
 * Initializes builtins table
 * returns 0 -> initialization was successful
 * return 1 -> initialization failed
 */
int init_Builtins()
{
    /* If builtins are already initialized */
    if(builtin_map) return 0;

    builtin_map = init_GenericMap(
        (unsigned int (*)(const void *))djb2_string_hash,
        (bool (*)(const void *, const void *))strings_equal,
        (void (*)(void *))NULL,
        (void (*)(void *))NULL);

    if (!builtin_map) {
        printf("Failed to initialize Built in functions \n");
        return 1;
    }

    map_insert(builtin_map, _builtin_print.builtin_name, (void*) &_builtin_print);
    map_insert(builtin_map, _builtin_println.builtin_name, (void*) &_builtin_println);

    return 0;
}

/**
 * Returns wether identifier is a built in identifier
*/
bool ident_is_builtin(const char *identifier)
{
    return map_contains_key(builtin_map, (void *)identifier);
}

/**
 * Returns wether built in object associated with identifier
 * Returns NULL if key value pair does not exist
*/
RtObject *get_builtin_func(const char *identifier) 
{
    Builtin *builtin = (Builtin*) map_get(builtin_map, (void*)identifier);
    if(!builtin) return NULL;
    RtObject *obj = init_RtObject(FUNCTION_TYPE);
    obj->data.Function.is_builtin=true;
    obj->data.Function.func_data.built_in.func=builtin;
    return obj;
}

/**
 * Cleans (i.e frees) builtin table
 */
void cleanup_builtin()
{
    free_GenericMap(builtin_map, false, false);
    builtin_map = NULL;
}

/**
 * Implementation for print built in function
 */
RtObject *builtin_print(const RtObject **args, int arg_count)
{
    for (int i = 0; i < arg_count; i++)
    {
        char *str = RtObject_to_String(args[i]);
        printf("%s ", str);
        free(str);
    }

    return init_RtObject(UNDEFINED_TYPE);
}

/**
 * Implementation for println built in function
 * Literally the exact same as print, but it prints out a new line
 */
RtObject *builtin_println(const RtObject **args, int arg_count)
{
    for (int i = 0; i < arg_count; i++)
    {
        char *str = RtObject_to_String(args[i]);
        printf("%s ", str);
        free(str);
    }
    printf("\n");

    return init_RtObject(UNDEFINED_TYPE);
}
