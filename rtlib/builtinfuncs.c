#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "builtinexception.h"
#include "../compiler/compiler.h"
#include "../generics/utilities.h"
#include "../generics/hashmap.h"
#include "../runtime/runtime.h"
#include "../runtime/rtobjects.h"
#include "../runtime/rttype.h"
#include "builtins.h"

/**
 * This file contains the implementation of all general built in functions:
 * - print
 * - println
 * - str
 * - typeof
 * - input
 * - num
 * - copy
 * - len
 * - cmd
 * - min
 * - max
 * - abs
 * - copy
 */

/**
 * CONVENTION:
 * If an exception occurs during the execution of a built in function
 * 1- raisedException variable is set to the relevant exception
 * 2- Function MUST return NULL
*/

#define BuiltinsInitError() exitprogram(FAILED_BUILTINS_INIT)

static RtObject *builtin_print(RtObject **args, int arg_count);
static RtObject *builtin_println(RtObject **args, int arg_count);
static RtObject *builtin_toString(RtObject **args, int arg_count);
static RtObject *builtin_typeof(RtObject **args, int arg_count);
static RtObject *builtin_input(RtObject **args, int arg_count);
static RtObject *builtin_toNumber(RtObject **args, int arg_count);
static RtObject *builtin_len(RtObject **args, int arg_count);
static RtObject *builtin_cmd(RtObject **args, int arg_count);
static RtObject *builtin_max(RtObject **args, int argcount);
static RtObject *builtin_min(RtObject **args, int argcount);
static RtObject *builtin_abs(RtObject **args, int argcount);
static RtObject *builtin_copy(RtObject **args, int argcount);

static GenericMap *BuiltinFunc_Registry = NULL;

static const BuiltinFunc _builtin_print = {"print", builtin_print, INT_FAST64_MAX};
static const BuiltinFunc _builtin_println = {"println", builtin_println, INT_FAST64_MAX};
static const BuiltinFunc _builtin_string = {"str", builtin_toString, INT_FAST64_MAX};
static const BuiltinFunc _builtin_typeof = {"typeof", builtin_typeof, 1};
static const BuiltinFunc _builtin_input = {"input", builtin_input, 1};
static const BuiltinFunc _builtin_number = {"num", builtin_toNumber, 1};
static const BuiltinFunc _builtin_len = {"len", builtin_len, 1};
static const BuiltinFunc _builtin_cmd = {"cmd", builtin_cmd, 1};
static const BuiltinFunc _builtin_min = {"min", builtin_min, INT64_MAX};
static const BuiltinFunc _builtin_max = {"max", builtin_max, INT_FAST64_MAX};
static const BuiltinFunc _builtin_abs = {"abs", builtin_abs, INT_FAST64_MAX};
static const BuiltinFunc _builtin_copy = {"copy", builtin_copy, INT_FAST64_MAX};

/**
 * Defines equality for built in functions
 */
static bool builtins_equal(const BuiltinFunc *builtin1, const BuiltinFunc *builtin2)
{
    return builtin1->arg_count == builtin2->arg_count &&
           builtin1->builtin_func == builtin2->builtin_func;
}

#define InsertBuiltIn(registry, builtin_struct) \
GenericHashMap_insert(registry, builtin_struct.builtin_name, (void *)&builtin_struct, false)

/**
 * Initializes builtin function table
 * returns 1 -> initialization was successful
 * return 0 -> initialization failed
 *
 * NOTE:
 * If lookup table is already initialized, function returns early
 */
int init_BuiltinFuncs()
{
    /* If builtins are already initialized */
    if (BuiltinFunc_Registry)
        return 1;

    BuiltinFunc_Registry = init_GenericMap(
        (unsigned int (*)(const void *))djb2_string_hash,
        (bool (*)(const void *, const void *))strings_equal,
        (void (*)(void *))NULL,
        (void (*)(void *))NULL);

    if (!BuiltinFunc_Registry)
    {
        printf("Failed to initialize Built in functions \n");
        return 0;
    }

    bool successful_init =
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_print) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_println) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_string) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_typeof) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_input) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_number) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_len) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_cmd) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_min) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_max) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_abs) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_copy) &&
        init_BuiltinException(BuiltinFunc_Registry);

    if (successful_init)
    {
        return 1;
    }
    else
    {
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
    if (!init_BuiltinFuncs())
        BuiltinsInitError();

    return GenericHashMap_contains_key(BuiltinFunc_Registry, (void *)identifier);
}

/**
 * DESCRIPTION:
 * Returns wether built in object associated with identifier
 * Returns NULL if key value pair does not exist
 */
RtObject *get_builtinfunc(const char *identifier)
{
    // lazy init
    // initialization fails, program exits
    if (!init_BuiltinFuncs())
        BuiltinsInitError();

    BuiltinFunc *builtin = (BuiltinFunc *)GenericHashMap_get(BuiltinFunc_Registry, (void *)identifier);
    if (!builtin)
        return NULL;

    RtObject *obj = init_RtObject(FUNCTION_TYPE);
    obj->data.Func = init_rtfunc(true);
    obj->data.Func->func_data.built_in.func = builtin;
    return obj;
}

/**
 * Cleans (i.e frees) builtin table
 */
void cleanup_builtin()
{
    free_GenericMap(BuiltinFunc_Registry, false, false);
    BuiltinFunc_Registry = NULL;
}

/**
 * Implementation for print built in function
 */
static RtObject *builtin_print(RtObject **args, int arg_count)
{
    for (int i = 0; i < arg_count; i++)
    {
        rtobj_print(args[i]);
        if (i != arg_count - 1)
            printf(" ");
    }

    return init_RtObject(UNDEFINED_TYPE);
}

/**
 * Implementation for println built in function
 * Literally the exact same as print, but it prints out a new line
 */
static RtObject *builtin_println(RtObject **args, int arg_count)
{
    for (int i = 0; i < arg_count; i++)
    {
        rtobj_print(args[i]);
        if (i != arg_count - 1)
            printf(" ");
    }
    printf("\n");

    return init_RtObject(UNDEFINED_TYPE);
}

/**
 * Implementation for string built in function
 * Converts a RtObject to a string
 */
static RtObject *builtin_toString(RtObject **args, int arg_count)
{
    char *str = malloc(sizeof(char));
    *str = '\0';

    for (int i = 0; i < arg_count; i++)
    {
        char *tmp = rtobj_toString(args[i]);
        char *new_str = concat_strings(tmp, str);
        free(str);
        free(tmp);
        str = new_str;
    }

    RtObject *string = init_RtObject(STRING_TYPE);
    string->data.String = init_RtString(NULL);
    string->data.String->string = str;
    string->data.String->length = strlen(str);
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
static RtObject *builtin_typeof(RtObject **args, int arg_count)
{
    if (arg_count != 1)
    {
        printf("typeof builtin function can only take 1 argument\n");
        return init_RtObject(UNDEFINED_TYPE);
    }

    RtObject *string_obj = init_RtObject(STRING_TYPE);
    const char *type = rtobj_type_toString(args[0]->type);
    string_obj->data.String = init_RtString(type);
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
static RtObject *builtin_input(RtObject **args, int arg_count)
{
    if (arg_count != 1)
    {
        printf("input builtin function can only take 1 argument\n");
        return init_RtObject(UNDEFINED_TYPE);
    }

    char *msg = rtobj_toString(args[0]);
    printf("%s", msg);
    free(msg);

    char buffer[1024];

    if (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strlen(buffer) - 1] = '\0'; // removes newline char
        RtObject *input = init_RtObject(STRING_TYPE);
        input->data.String = init_RtString(buffer);
        return input;
    }
    else
    {
        printf("Error reading input \n");
        return init_RtObject(UNDEFINED_TYPE);
    }
}

/**
 * DESCRIPTION:
 * Converts runtime object to
 */
static RtObject *builtin_toNumber(RtObject **args, int arg_count)
{
    if (arg_count != 1)
    {
        printf("input builtin function can only take 1 argument\n");
        return init_RtObject(UNDEFINED_TYPE);
    }

    if (args[0]->type == NUMBER_TYPE)
    {
        RtObject *num = init_RtObject(NUMBER_TYPE);
        num->data.Number = args[0]->data.Number;
        return num;
    }
    else if (args[0]->type == STRING_TYPE)
    {
        char *str = args[0]->data.String->string;
        if (is_token_numeric(str))
        {
            RtObject *num = init_RtObject(NUMBER_TYPE);
            num->data.Number->number = atof(str);
            return num;
        }
        else
        {
            printf("String '%s' cannot be converted to a number\n", str);
            return init_RtObject(UNDEFINED_TYPE);
        }
    }
    else
    {
        printf("%s type cannot be converted to a int\n", rtobj_type_toString(args[0]->type));
        return init_RtObject(UNDEFINED_TYPE);
    }
}

/**
 * DESCRIPTION:
 * Implentation of the len built in function for getting the length of objects
 * This function only expects 1 argument
 *
 * PARAMS:
 * args: argument objects
 * arg_count: # of argument
 */
static RtObject *builtin_len(RtObject **args, int arg_count)
{
    if (arg_count != 1)
    {
        printf("len builtin function can only take 1 argument \n");
        return init_RtObject(UNDEFINED_TYPE);
    }
    RtObject *arg = args[0];
    assert(arg);
    switch (arg->type)
    {
    case HASHMAP_TYPE:
    {
        RtObject *length = init_RtObject(NUMBER_TYPE);
        length->data.Number = init_RtNumber(arg->data.Map->size);
        return length;
    }

    case LIST_TYPE:
    {
        RtObject *length = init_RtObject(NUMBER_TYPE);
        length->data.Number = init_RtNumber(arg->data.List->length);
        return length;
    }

    case STRING_TYPE:
    {
        RtObject *length = init_RtObject(NUMBER_TYPE);
        length->data.Number = init_RtNumber(arg->data.String->length);
        return length;
    }

    case HASHSET_TYPE:
    {
        RtObject *length = init_RtObject(NUMBER_TYPE);
        length->data.Number = init_RtNumber(arg->data.Set->size);
        return length;
    }

    case CLASS_TYPE:
    {
        // UNDER CONSTRUCTION
        // RtObject *length = init_RtObject(NUMBER_TYPE);
        // length->data.Number = init_RtNumber(0);
        printf("Cannot get length of Class type\n");
        return init_RtObject(UNDEFINED_TYPE);
    }

    default:
        printf("Cannot get length of object of type %s", rtobj_type_toString(arg->type));
        return init_RtObject(UNDEFINED_TYPE);
    }
}

/**
 * DESCRIPTION:
 * Built in function for running commands in the command line
 */
static RtObject *builtin_cmd(RtObject **args, int arg_count)
{
    if (arg_count != 1)
    {
        printf("Built in function cmd must take exactly one argument\n");
        return init_RtObject(UNDEFINED_TYPE);
    }

    if (args[0]->type != STRING_TYPE)
    {
        printf("Built in function cmd must take a string type argument\n");
        return init_RtObject(UNDEFINED_TYPE);
    }

    char stdoutbuffer[5000];
    FILE *fp = NULL;
    char *command = args[0]->data.String->string;
    char *output = NULL;
    size_t output_size = 0;
    size_t bytes_read;

    /* Open the command for reading. */
    fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("Failed to run command '%s'\n", command);
        return init_RtObject(UNDEFINED_TYPE);
    }

    // Read the entire command output
    while ((bytes_read = fread(stdoutbuffer, 1, sizeof(stdoutbuffer), fp)) > 0)
    {
        // Resize output buffer
        output_size += bytes_read;
        output = realloc(output, output_size + 1);
        if (output == NULL)
        {
            perror("Memory allocation failed");
            pclose(fp);
            return NULL;
        }

        // Append current chunk to output
        memcpy(output + output_size - bytes_read, stdoutbuffer, bytes_read);
    }

    pclose(fp);

    system(command);

    RtObject *stdout_ = init_RtObject(STRING_TYPE);
    stdout_->data.String = init_RtString(NULL);
    stdout_->data.String->string = output;

    return stdout_;
}

/**
 * DESCRIPTION:
 * Built in function for getting the max value
 *
 */
static RtObject *builtin_max(RtObject **args, int argcount)
{
    if(argcount == 0) {
        assert(false);

    }

    RtObject *max = args[0];
    for (int i = 1; i < argcount; i++)
    {
        if (rtobj_compare(max, args[i]) < 0)
        {
            max = args[i];
        }
    }

    return max;
}

/**
 * DESCRIPTION:
 * Built in function for getting the min value of a list
 *
 */
static RtObject *builtin_min(RtObject **args, int argcount)
{
    // temporary
    assert(argcount > 0);

    RtObject *max = args[0];
    for (int i = 1; i < argcount; i++)
    {
        if (rtobj_compare(max, args[i]) > 0)
        {
            max = args[i];
        }
    }

    return max;
}

/**
 * DESCRIPTION:
 * Built in function for computing the absolute value of a number
 *
 */
static RtObject *builtin_abs(RtObject **args, int argcount)
{
    // temporary
    assert(argcount == 1);
    assert(args[0]->type == NUMBER_TYPE);
    RtNumber *num = init_RtNumber(fabsl(args[0]->data.Number->number));
    RtObject *obj = init_RtObject(NUMBER_TYPE);
    obj->data.Number = num;
    return obj;
}

/**
 * DESCRIPTION:
 * Built in function for creating a deep copy of a runtime object
 */
static RtObject *builtin_copy(RtObject **args, int argcount)
{
    // temporary
    assert(argcount == 1);
    // objects is deep copied and ALL objects contained by that object are also deep copied
    // therefore, all of those objects are added to the GC
    RtObject *obj = rtobj_deep_cpy(args[0], true);
    return obj;
}
