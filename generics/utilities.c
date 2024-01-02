#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "utilities.h"

/**
 * DESCRIPTION:
 * File contains useful set of general purpose functions
 * */

/**
 * DESCRIPTION:
 * Checks if string is an integer, string must not have whitespace
 * 
 * PARAMS:
 * token: string to parse
 * */
bool is_token_integer(char *token)
{
    int i = 0;
    if (token[0] == '-' || token[0] == '+')
        i++;

    for (; i < (int)strlen(token); i++)
    {
        if (!isdigit(token[i]))
            return false;
    }
    return true;
}

/**
 * DESCRIPTION:
 * Checks if string is an numeric value, can be a double, string cant have whitespace
 * 
 * PARAMS:
 * token: string to parse
 * */
bool is_token_numeric(char *token)
{
    int i = 0;
    if (token[0] == '-' || token[0] == '+')
        i++;

    bool encountered_dot = false;
    for (; i < (int)strlen(token); i++)
    {
        if(token[i] == '.' && !encountered_dot) {
            encountered_dot=true;
            continue;
        }

        if (!isdigit(token[i]))
            return false;
    }
    return true;
}

/**
 * DESCRIPTION:
 * Forcefully exits program with given error enum (i.e int)
 * Useful for handling errors that cant be recovered from.
 */
void exitprogram(ErrorCode code)
{
    switch (code)
    {
    case FAILED_MEMORY_ALLOCATION:
        printf("MEMORY ERROR: memory allocation return NULL");
        break;
    default:
        printf("Exited with error %d", (int)code);
        break;
    }
    exit(code);
}

/***** Set of useful functions for using hashmap/hashset with primitive types ******/

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Useful functions for dynamically copying strings
 *
 * PARAMS:
 * str: string to copy
 */
char *
cpy_string(const char *str)
{
    char *cpy = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(cpy, str);
    return cpy;
}

/**
 * DESCRIPTION:
 * Hash function for strings
 * PARAMS:
 * str: string to be hashed
 */
unsigned int djb2_string_hash(const char *str)
{
    assert(str);
    unsigned int hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

/**
 * DESCRIPTION:
 * Hash function for integers
 *
 * PARAMS:
 * integer: int to be hashed
 */
unsigned int hash_int(const int *integer)
{
    assert(integer);
    const unsigned int A = 2654435769; // A is a constant fraction close to (sqrt(5) - 1) / 2
    return (unsigned int)(A * (*integer));
}

/**
 * DESCRIPTION:
 * Function for checking if two strings are equal
 *
 * PARAMS:
 * str1 and str2: 2 strings that will be compared
 */
bool strings_equal(const char *str1, const char *str2)
{
    if (!str1 && !str2)
        return true;
    if (!str1 || !str2)
        return false;

    int i = 0;
    for (; str1[i] != '\0' && str2[i] != '\0'; i++)
    {
        if (str1[i] != str2[i])
        {
            return false;
        }
    }

    if (str1[i] == '\0' && str2[i] == '\0')
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * DESCRIPTION:
 * Returns the length of a NULL terminated generic 2D Array
 *
 * PARAMS:
 * arr: array to get length from
 *
 * IMPORTANT:
 * Array MUST be NULL TERMINATED, otherwise it leads to undefined behaviour
 */

int get_pointer_list_length(void **arr)
{
    int length = 0;
    for (int i = 0; arr[i] != NULL; i++)
        length++;
    return length;
}

/**
 * DESCRIPTION:
 * Takes 2 integer pointers and checks if both are equal
 * If either one is NULL, function returns false
 *
 * PARAMS:
 * integer1: int
 * integer2: int
 *
 * NOTE:
 * These type of functions are useful for using the Generic map or set
 */
bool integers_equal(const int *integer1, const int *integer2)
{
    if (integer1 == NULL || integer2 == NULL)
        return false;
    return (*integer1) == (*integer2);
}

/**
 * DESCRIPTION:
 * Simple function for comparing if an int is larger or equal to the static integer variable
 * PARAMS:
 * integer: int to be compared
 */
static int _compare_val = 0;
static bool _integer_filter(const int *integer) { return (*integer) >= _compare_val; }

/**
 * DESCRIPTION:
 * Returns a function pointer that will check if int is larger/equal to the 'cutoff'
 *
 * PARAMS:
 * cutoff: what value should the function check with
 *
 * */
typedef bool (*IntFilter)(const int *);
IntFilter integer_bge_than(int cutoff)
{
    _compare_val = cutoff;
    return _integer_filter;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Dynamically concatenates two strings, returned string is allocated on the heap
 * input strings are NOT freed
 *
 * One of the input strings can be null, but AT LEAST one must not be NULL
 *
 * PARAMS:
 * str1, str2: strings to be concatenated
 *
 * NOTE:
 * Returns NULL if malloc returns NULL
 */
char *
concat_strings(char *str1, char *str2)
{
    assert(str1 || str2);

    // If either string is NULL, return a copy of the non-NULL string
    if (!str1)
        return cpy_string(str2);
    if (!str2)
        return cpy_string(str1);

    size_t len = strlen(str1) + strlen(str2);
    char *concat_str = malloc(len + 1);
    if (!concat_str)
        return NULL;

    strcpy(concat_str, str1);
    strcat(concat_str, str2);

    return concat_str;
}