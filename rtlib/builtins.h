#pragma once

typedef struct RtObject RtObject;

/**
 * Represents a Builtin Function
 */
typedef struct Builtin
{
    // name of built in function
    char *builtin_name;

    RtObject *(*builtin_func)(RtObject **, int);

    int arg_count; // if its -1, then functions takes any amount of arguments
} Builtin;

int init_Builtins();
bool ident_is_builtin(const char *identifier);
RtObject *get_builtin_func(const char *identifier);
void cleanup_builtin();
