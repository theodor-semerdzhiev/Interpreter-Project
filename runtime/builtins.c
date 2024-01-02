#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../compiler/compiler.h"
#include "../generics/utilities.h"
#include "../generics/hashmap.h"
#include "runtime.h"
#include "rtobjects.h"
#include "builtins.h"

/**
 * This file contains the implementation of all general built in functions:
 * - print
 *
 */

#define BuiltinsInitError() exitprogram(FAILED_BUILTINS_INIT)

static GenericMap *builtin_map = NULL;

/* Print function */
static const Builtin _builtin_print = {"print", builtin_print, -1};
static const Builtin _builtin_println = {"println", builtin_println, -1};
static const Builtin _builtin_string = {"Str", builtin_toString, -1};
static const Builtin _builtin_typeof = {"Typeof", builtin_typeof, 1};
static const Builtin _builtin_input = {"input", builtin_input, 1};
static const Builtin _builtin_number = {"Number", builtin_toNumber, 1};

/**
 * Defines equality for built in functions
 */
static bool builtins_equal(const Builtin *builtin1, const Builtin *builtin2)
{
    return builtin1->arg_count == builtin2->arg_count && builtin1->builtin_func == builtin2->builtin_func;
}

/**
 * Initializes builtins table
 * returns 1 -> initialization was successful
 * return 0 -> initialization failed
 * 
 * NOTE:
 * If lookup table is already initialized, function returns early
 */
int init_Builtins()
{
    /* If builtins are already initialized */
    if (builtin_map)
        return 1;

    builtin_map = init_GenericMap(
        (unsigned int (*)(const void *))djb2_string_hash,
        (bool (*)(const void *, const void *))strings_equal,
        (void (*)(void *))NULL,
        (void (*)(void *))NULL);

    if (!builtin_map)
    {
        printf("Failed to initialize Built in functions \n");
        return 0;
    }

    if(
    GenericHashMap_insert(builtin_map, _builtin_print.builtin_name, (void *)&_builtin_print, false) &&
    GenericHashMap_insert(builtin_map, _builtin_println.builtin_name, (void *)&_builtin_println, false) &&
    GenericHashMap_insert(builtin_map, _builtin_string.builtin_name, (void *)&_builtin_string, false) &&
    GenericHashMap_insert(builtin_map, _builtin_typeof.builtin_name, (void *)&_builtin_typeof, false) &&
    GenericHashMap_insert(builtin_map, _builtin_input.builtin_name, (void *)&_builtin_input, false) &&
    GenericHashMap_insert(builtin_map, _builtin_number.builtin_name, (void *)&_builtin_number, false)) {
        return 1;
    } else {
        printf("Failed to initialize Built in functions \n");
        cleanup_builtin();
        return 0;
    }
}

/**
 * Returns wether string is a built in identifier
 * If builtin map is not initializes, its initialized, if this fails, program is forcefully terminated
 */
bool ident_is_builtin(const char *identifier)
{
    // lazy init
    // initialization fails, program exits
    if(!init_Builtins()) BuiltinsInitError();
    
    return GenericHashMap_contains_key(builtin_map, (void *)identifier);
}

/**
 * Returns wether built in object associated with identifier
 * Returns NULL if key value pair does not exist
 */
RtObject *get_builtin_func(const char *identifier)
{
    // lazy init
    // initialization fails, program exits
    if(!init_Builtins()) BuiltinsInitError();

    Builtin *builtin = (Builtin *)GenericHashMap_get(builtin_map, (void *)identifier);
    if (!builtin)
        return NULL;
    RtObject *obj = init_RtObject(FUNCTION_TYPE);
    obj->data.Function.is_builtin = true;
    obj->data.Function.func_data.built_in.func = builtin;
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

/**
 * Implementation for string built in function
 * Converts a RtObject to a string
 */
RtObject *builtin_toString(const RtObject **args, int arg_count)
{
    char *str = malloc(sizeof(char));
    *str = '\0';

    for (int i = 0; i < arg_count; i++)
    {
        char *tmp = RtObject_to_String(args[i]);
        char *new_str = malloc(sizeof(char) * (strlen(tmp) + strlen(str) + 1));
        strcat(new_str, tmp);
        strcat(new_str, str);
        new_str[strlen(tmp) + strlen(str)] = '\0';
        free(str);
        free(tmp);
        str = new_str;
    }

    RtObject *string = init_RtObject(STRING_TYPE);
    string->data.String.string = str;
    string->data.String.string_length = strlen(str);
    return string;
}

/**
 * DESCRIPTION:
 * Implementation for the typeof built in function
 * It returns string representation of the type of the object
 *
 * PARAMS:
 * obj: input to function
 */
RtObject *builtin_typeof(const RtObject **args, int arg_count)
{
    if(arg_count == 0) {
        return init_RtObject(UNDEFINED_TYPE);
    }
    if(arg_count > 1) {
        printf("typeof builtin function can only take 1 argument\n");
        return init_RtObject(UNDEFINED_TYPE);
    }

    RtObject *string_obj = init_RtObject(STRING_TYPE);
    const char *type = obj_type_toString(args[0]);
    string_obj->data.String.string = cpy_string(type);
    string_obj->data.String.string_length = strlen(type);
    return string_obj;
}

/**
 * DESCRIPTION:
 * Implementation of a input function for getting input from user
 * 
 * PARAMS:
 * args: arguments to function
 * arg_count: number of arguments
*/
RtObject *builtin_input(const RtObject **args, int arg_count) {
    if(arg_count > 1) {
        printf("input builtin function can only take 1 argument\n");
        return init_RtObject(UNDEFINED_TYPE);
    }

    char* msg = RtObject_to_String(args[0]);
    printf("%s", msg);
    free(msg);
    
    char buffer[1024];

    if(fgets(buffer, sizeof(buffer), stdin) != NULL) {
        buffer[strlen(buffer)-1]='\0'; // removes newline char
        RtObject *input = init_RtObject(STRING_TYPE);
        input->data.String.string=cpy_string(buffer);
        return input;
    } else {
        printf("Error reading input \n");
        return init_RtObject(UNDEFINED_TYPE);
    }
}

/**
 * DESCRIPTION:
 * Converts runtime object to 
*/
RtObject *builtin_toNumber(const RtObject **args, int arg_count) {
    if(arg_count != 1) {
        printf("input builtin function can only take 1 argument\n");
        return init_RtObject(UNDEFINED_TYPE);
    }

    if(args[0]->type == NUMBER_TYPE) {
        RtObject* num = init_RtObject(NUMBER_TYPE);
        num->data.Number.number=args[0]->data.Number.number;
        return num;
    } else if(args[0]->type == STRING_TYPE) {
        char * str = args[0]->data.String.string;
        if(is_token_numeric(str)) {
            RtObject *num = init_RtObject(NUMBER_TYPE);
            num->data.Number.number = atof(str);
            return num;
        } else {
            printf("String '%s' cannot be converted to a number\n", str);
            return init_RtObject(UNDEFINED_TYPE);
        }
    } else {
        printf("%s type cannot be converted to a int\n", obj_type_toString(args[0]));
        return init_RtObject(UNDEFINED_TYPE);
    }

}
