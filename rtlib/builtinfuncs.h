#pragma once
#include <stddef.h>
typedef struct RtObject RtObject;

/**
 * Represents a Builtin Function
 */
typedef struct BuiltinFunc
{
    // name of built in function
    char *builtin_name;

    RtObject *(*builtin_func)(RtObject **, int);

    size_t arg_count; // if its -1, then functions takes any amount of arguments

} BuiltinFunc;

int init_BuiltinFuncs();
bool ident_is_builtin(const char *identifier);
RtObject *get_builtinfunc(const char *identifier);
void cleanup_builtin();

#define BUILT_IN_SCRIPT_ARGS_VAR "_args"

