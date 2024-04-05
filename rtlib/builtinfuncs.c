#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "builtinexception.h"
#include "builtins.h"
#include "../compiler/compiler.h"
#include "../generics/utilities.h"
#include "../generics/hashmap.h"
#include "../runtime/runtime.h"
#include "../runtime/rtobjects.h"
#include "../runtime/rttype.h"
#include "../runtime/rtexchandler.h"

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
 * - floor
 * - round
 * - ciel
 * - read
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
static RtObject *builtin_ord(RtObject **args, int argcount);
static RtObject *builtin_floor(RtObject **args, int argcount);
static RtObject *builtin_round(RtObject **args, int argcount);
static RtObject *builtin_ciel(RtObject **args, int argcount);
static RtObject *builtin_read(RtObject **args, int argcount);

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
static const BuiltinFunc _builtin_abs = {"abs", builtin_abs, 1};
static const BuiltinFunc _builtin_copy = {"copy", builtin_copy, 1};
static const BuiltinFunc _builtin_ord = {"ord", builtin_ord, 1};
static const BuiltinFunc _builtin_floor = {"floor", builtin_floor, 1};
static const BuiltinFunc _builtin_round = {"round", builtin_round, 1};
static const BuiltinFunc _builtin_ciel = {"ciel", builtin_ciel, 1};
static const BuiltinFunc _builtin_read = {"read", builtin_read, 1};

#define setInvalidNumberOfArgsIntermediateException(built_name, actual_args, expected_args) \
    setIntermediateException(init_InvalidNumberOfArgumentsException(built_name, actual_args, expected_args))

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
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_ord) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_floor) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_round) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_ciel) &&
        InsertBuiltIn(BuiltinFunc_Registry, _builtin_read) &&
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
 * DESCRIPTION:
 * Implementation for string built in function
 * Converts a RtObject to a string
 */
static RtObject *builtin_toString(RtObject **args, int arg_count)
{
    if(arg_count != 1) {
        setInvalidNumberOfArgsIntermediateException("Builtin str()", arg_count, 1)
        return NULL;
    }

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
        setInvalidNumberOfArgsIntermediateException("Builtin typeof()", arg_count, 1);
        return NULL;
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
        setInvalidNumberOfArgsIntermediateException("Builtin input()", arg_count, 1);
        return NULL;
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
        setIntermediateException(IOExceptionException("Error occured trying to fetch Standard Input."));
        return NULL;
    }
}

/**
 * DESCRIPTION:
 * Helper Function for creating exception for when num() is enable to convert object to number
*/
static RtException *_CannotConvertObjToNumber(RtObject *invalid_arg) {
    assert(invalid_arg);
    const char *type = rtobj_type_toString(invalid_arg->type);
    char *argtostr = rtobj_toString(invalid_arg);
    char buffer[100 + strlen(type) + strlen(argtostr)];
    snprintf(buffer, sizeof(buffer), 
        "Builtin num() cannot convert Object %s with type %s to a Number",
        argtostr, type
    );

    free(argtostr);

    return InvalidValueException(buffer);
}

/**
 * DESCRIPTION:
 * Converts runtime object to
 */
static RtObject *builtin_toNumber(RtObject **args, int arg_count)
{
    if (arg_count != 1)
    {
        setInvalidNumberOfArgsIntermediateException("Builtin num()", arg_count, 1);
        return NULL;
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
            setIntermediateException(_CannotConvertObjToNumber(args[0]));
            return NULL;
        }
    }
    else
    {
        setIntermediateException(_CannotConvertObjToNumber(args[0]));
        return NULL;
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
        setInvalidNumberOfArgsIntermediateException("Builtin len()", arg_count, 1);
        return NULL;
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

    default:
        setIntermediateException(
            init_InvalidTypeException_Builtin("len()", "Map, List, String, or Set", arg)
        );
        return NULL;
    }
}

#define MAX_STDOUT_BUFFER 6000
/**
 * DESCRIPTION:
 * Built in function for running commands in the command line
 */
static RtObject *builtin_cmd(RtObject **args, int arg_count)
{
    if (arg_count != 1)
    {
        setInvalidNumberOfArgsIntermediateException("Builtin cmd()", arg_count, 1);
        return NULL;
    }

    if (args[0]->type != STRING_TYPE)
    {
        setIntermediateException(init_InvalidTypeException_Builtin("cmd()", "String", args[0]));
        return NULL;
    }

    char stdoutbuffer[MAX_STDOUT_BUFFER];
    FILE *fp = NULL;
    const char *command = args[0]->data.String->string;
    char *output = NULL;
    size_t output_size = 0;
    size_t bytes_read;

    /* Open the command for reading. */
    fp = popen(command, "r");
    if (fp == NULL)
    {
        char buffer[60 + strlen(command)];
        snprintf(buffer, sizeof(buffer), "Builtin function cmd() failed to run command '%s'", command);
        setIntermediateException(IOExceptionException(buffer));
        return NULL;
    }

    // Read the entire command output
    while ((bytes_read = fread(stdoutbuffer, 1, sizeof(stdoutbuffer), fp)) > 0)
    {
        // Resize output buffer
        output_size += bytes_read;
        output = realloc(output, output_size + 1);
        if (output == NULL)
        {
            char buffer[80];
            snprintf(buffer, sizeof(buffer), "Builtin function cmd() failed to run command due to memory allocation error.");
            setIntermediateException(IOExceptionException(buffer));
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
        setInvalidNumberOfArgsIntermediateException("Builtin max()", 0, INT64_MAX)
        return NULL;
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
    if(argcount == 0) {
        setInvalidNumberOfArgsIntermediateException("Builtin min()", argcount, INT64_MAX)
        return NULL;
    }

    RtObject *min = args[0];
    for (int i = 1; i < argcount; i++)
    {
        if (rtobj_compare(min, args[i]) > 0)
        {
            min = args[i];
        }
    }

    return min;
}

/**
 * DESCRIPTION:
 * Built in function for computing the absolute value of a number
 *
 */
static RtObject *builtin_abs(RtObject **args, int argcount)
{
    if(argcount != 1) {
        setInvalidNumberOfArgsIntermediateException("Builtin abs()", argcount, 1)
        return NULL;
    }

    if(args[0]->type != NUMBER_TYPE) {
        setIntermediateException(init_InvalidTypeException_Builtin("abs()", "Number", args[0])); 
        return NULL;
    }

    RtNumber *num = init_RtNumber(fabsl(args[0]->data.Number->number));
    RtObject *obj = init_RtObject(NUMBER_TYPE);
    obj->data.Number = num;
    return obj;
}

/**
 * DESCRIPTION:
 * Built in function for creating a shallow copy of a runtime object
 */
static RtObject *builtin_copy(RtObject **args, int argcount)
{
    if(argcount != 1) {
        setInvalidNumberOfArgsIntermediateException("Builtin copy()", argcount, 1);
        return NULL;
    }

    RtObject *obj = rtobj_shallow_cpy(args[0]);
    return obj;
}


/**
 * DESCRIPTION:
 * Built in function for getting value of a one character string (char -> int)
 * 
 * This function ONLY takes the first char of a string
*/
static RtObject *builtin_ord(RtObject **args, int argcount) {
    if (argcount != 1)
    {
        setInvalidNumberOfArgsIntermediateException("Builtin ord()", argcount, 1);
        return NULL;
    }
    if(args[0]->type != STRING_TYPE) {
        setIntermediateException(init_InvalidTypeException_Builtin("ord()", "String", args[0])); 
        return NULL;
    }
    RtString *string = args[0]->data.String;
    
    char c = args[0]->data.String->string[0];
    RtObject *ord = init_RtObject(NUMBER_TYPE);
    ord->data.Number = init_RtNumber(c);
    return ord;
}

/**
 * DESCRIPTION:
 * Built in function for flooring numbers
 * 
 * This function ONLY takes the first char of a string
*/
static RtObject *builtin_floor(RtObject **args, int argcount) {
    if (argcount != 1)
    {
        setInvalidNumberOfArgsIntermediateException("Builtin floor()", argcount, 1);
        return NULL;
    }

    if(args[0]->type != NUMBER_TYPE) {
        setIntermediateException(init_InvalidTypeException_Builtin("floor()", "Number", args[0])); 
        return NULL;
    }
    
    RtObject *floored = init_RtObject(NUMBER_TYPE);
    floored->data.Number = init_RtNumber(floorl(args[0]->data.Number->number));
    return floored;
}

/**
 * DESCRIPTION:
 * Built in function for rounding numbers
 * 
 * This function ONLY takes the first char of a string
*/
static RtObject *builtin_round(RtObject **args, int argcount) {
    if (argcount != 1)
    {
        setInvalidNumberOfArgsIntermediateException("Builtin round()", argcount, 1);
        return NULL;
    }
    
    if(args[0]->type != NUMBER_TYPE) {
        setIntermediateException(init_InvalidTypeException_Builtin("round()", "Number", args[0])); 
        return NULL;
    }
    
    RtObject *rounded = init_RtObject(NUMBER_TYPE);
    rounded->data.Number = init_RtNumber(roundl(args[0]->data.Number->number));
    return rounded;
}

/**
 * DESCRIPTION:
 * Built in function for rounding numbers UP
 * 
 * This function ONLY takes the first char of a string
*/
static RtObject *builtin_ciel(RtObject **args, int argcount) {
    if (argcount != 1)
    {
        setInvalidNumberOfArgsIntermediateException("Builtin ciel()", argcount, 1);
        return NULL;
    }
    
    if(args[0]->type != NUMBER_TYPE) {
        setIntermediateException(init_InvalidTypeException_Builtin("ciel()", "Number", args[0])); 
        return NULL;
    }
    
    RtObject *cieled = init_RtObject(NUMBER_TYPE);
    cieled->data.Number = init_RtNumber(ceill(args[0]->data.Number->number));
    return cieled;
}

/**
 * DESCRIPTION:
 * Builtin function for reading entire file and returning its contents
*/
static RtObject *builtin_read(RtObject **args, int argcount) {
    if(argcount != 1) {
        setInvalidNumberOfArgsIntermediateException("read()", argcount, 1);
        return NULL;
    } 

    if(args[0]->type != STRING_TYPE) {
        setIntermediateException(init_InvalidTypeException_Builtin("read()", "String", args[0]));
        return NULL;
    }

    char *filename = args[0]->data.String->string;
    size_t filename_length = args[0]->data.String->length;

    FILE *file;
    if ((file = fopen(filename, "r")) == NULL)
    {
        char buffer[100 + 2 * filename_length];
        snprintf(buffer, sizeof(buffer), "Builtin function read() failed to read file %s, because %s does not exist.", filename, filename);
        setIntermediateException(IOExceptionException(buffer));
        return NULL;
    }
    fclose(file);

    char* file_contents = get_file_contents(filename);
    if(!file_contents) {
        char buffer[100 + filename_length];
        snprintf(buffer, sizeof(buffer), "Builtin function read() failed to read file %s, even though the target file exists.", filename);
        setIntermediateException(IOExceptionException(buffer));
        return NULL;
    }

    RtObject *readfile = init_RtObject(STRING_TYPE);
    readfile->data.String = init_RtString(file_contents);

    return readfile;
}