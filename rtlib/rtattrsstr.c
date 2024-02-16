#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "rtattrs.h"
#include "../generics/hashmap.h"
#include "../generics/utilities.h"

/**
 * DESCRIPTION:
 * This file contains all the implementations for all built in string functions, this includes:
 * 
 * upper(): Creates new string to all upper string
 * lower(): Creates new string to all lower case
 * strip(): Creates new string stripped of trailing and leading whitespace
 * find("..."): Finds the first occurence of a sub string, if none is found, -1 is returned
 * isalnum(): Returns wether string contains ONLY alpha numeric characters
 * isnumeric(): Returns wether string contains ONLY numeric characters
 * isalph(): Returns wether string contains ONLY alphabetical characters
 * isspace(): Returns wether string contains ONLY whitespace
*/

static RtObject *builtin_str_upper(RtObject *target, RtObject **args, int arg_count);
static RtObject *builtin_str_lower(RtObject *target, RtObject **args, int arg_count);
static RtObject *builtin_str_strip(RtObject *target, RtObject **args, int arg_count);
static RtObject *builtin_str_find(RtObject *target, RtObject **args, int arg_count);
static RtObject *builtin_str_isalnum(RtObject *target, RtObject **args, int arg_count);
static RtObject *builtin_str_isnumeric(RtObject *target, RtObject **args, int arg_count);
static RtObject *builtin_str_isalph(RtObject *target, RtObject **args, int arg_count);
static RtObject *builtin_str_isspace(RtObject *target, RtObject **args, int arg_count);
static RtObject *builtin_str_isupper(RtObject *target, RtObject **args, int arg_count);
static RtObject *builtin_str_islower(RtObject *target, RtObject **args, int arg_count);

static const AttrBuiltinKey _str_upper_key = {STRING_TYPE, "upper"};
static const AttrBuiltin _str_upper =
    {STRING_TYPE, {.builtin_func = builtin_str_upper}, 0, "upper", true};

static const AttrBuiltinKey _str_lower_key = {STRING_TYPE, "lower"};
static const AttrBuiltin _str_lower =
    {STRING_TYPE, {.builtin_func = builtin_str_lower}, 0, "lower", true};

static const AttrBuiltinKey _str_strip_key = {STRING_TYPE, "strip"};
static const AttrBuiltin _str_strip =
    {STRING_TYPE, {.builtin_func = builtin_str_strip}, 0, "strip", true};

static const AttrBuiltinKey _str_find_key = {STRING_TYPE, "find"};
static const AttrBuiltin _str_find =
    {STRING_TYPE, {.builtin_func = builtin_str_find}, 0, "find", true};

static const AttrBuiltinKey _str_isalnum_key = {STRING_TYPE, "isalnum"};
static const AttrBuiltin _str_isalnum =
    {STRING_TYPE, {.builtin_func = builtin_str_isalnum}, 0, "isalnum", true};

static const AttrBuiltinKey _str_isnumeric_key = {STRING_TYPE, "isnumeric"};
static const AttrBuiltin _str_isnumeric =
    {STRING_TYPE, {.builtin_func = builtin_str_isnumeric}, 0, "isnumeric", true};

static const AttrBuiltinKey _str_isalph_key = {STRING_TYPE, "isalph"};
static const AttrBuiltin _str_isalph =
    {STRING_TYPE, {.builtin_func = builtin_str_isalph}, 0, "isalph", true};

static const AttrBuiltinKey _str_isspace_key = {STRING_TYPE, "isspace"};
static const AttrBuiltin _str_isspace =
    {STRING_TYPE, {.builtin_func = builtin_str_isspace}, 0, "isspace", true};

static const AttrBuiltinKey _str_isupper_key = {STRING_TYPE, "isupper"};
static const AttrBuiltin _str_isupper =
    {STRING_TYPE, {.builtin_func = builtin_str_isupper}, 0, "isupper", true};

static const AttrBuiltinKey _str_islower_key = {STRING_TYPE, "islower"};
static const AttrBuiltin _str_islower =
    {STRING_TYPE, {.builtin_func = builtin_str_islower}, 0, "islower", true};

/**
 * DESCRIPTION:
 * Useful macro for applying a true/false check on a string
 * Result of that check gets stored in bool_
*/
#define StrBoolPred(bool, str, length, predicate) \
for(unsigned int i=0; i < length; i++) { \
    if(!predicate(str[i])) { bool = false; break;} \
} 

/**
 * DESCRIPTION:
 * Useful macro for initialization a new string on some predicate function
 * Also sets the null char
*/
#define NewStrApply(newstr, str, length, predicate) \
for(unsigned int i=0; i < length; i++) { \
    newstr[i]=predicate(str[i]); \
} newstr[len]='\0'; \

/**
 * DESCRIPTION:
 * Inserts String built in functions into registry
 */
void init_RtStrAttr(GenericMap *registry)
{
    addToAttrRegistry(registry, _str_upper_key, _str_upper);
    addToAttrRegistry(registry, _str_lower_key, _str_lower);
    addToAttrRegistry(registry, _str_strip_key, _str_strip);
    addToAttrRegistry(registry, _str_find_key, _str_find);
    addToAttrRegistry(registry, _str_isalnum_key, _str_isalnum);
    addToAttrRegistry(registry, _str_isnumeric_key, _str_isnumeric);
    addToAttrRegistry(registry, _str_isalph_key, _str_isalph);
    addToAttrRegistry(registry, _str_isspace_key, _str_isspace);
    addToAttrRegistry(registry, _str_isupper_key, _str_isupper);
    addToAttrRegistry(registry, _str_islower_key, _str_islower);
}

/**
 * DESCRIPTION:
 * Built in function for converting string to upper case
 */
static RtObject *builtin_str_upper(RtObject *target, RtObject **args, int arg_count)
{
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 0);
    (void)args;
    char *str = target->data.String->string;
    unsigned int len = target->data.String->length;
    char *newstr = malloc(sizeof(char)*(len + 1));
    NewStrApply(newstr, str, len, toupper);
    RtObject *strobj = init_RtObject(STRING_TYPE);
    strobj->data.String=init_RtString(NULL);
    strobj->data.String->string=newstr;
    strobj->data.String->length=len;
    return strobj;
}

/**
 * DESCRIPTION:
 * Built in function for converting string to lower case
 */
static RtObject *builtin_str_lower(RtObject *target, RtObject **args, int arg_count)
{
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 0);
    (void)args;
    char *str = target->data.String->string;
    unsigned int len = target->data.String->length;
    char *newstr = malloc(sizeof(char)*(len + 1));
    NewStrApply(newstr, str, len, tolower);
    RtObject *strobj = init_RtObject(STRING_TYPE);
    strobj->data.String=init_RtString(NULL);
    strobj->data.String->string=newstr;
    strobj->data.String->length=len;
    return strobj;
}

/**
 * DESCRIPTION:
 * Built in function for stripping a string of its whitespace
*/
static RtObject *builtin_str_strip(RtObject *target, RtObject **args, int arg_count) {
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 0);
    (void)args;
    char *str = target->data.String->string;
    unsigned int len = target->data.String->length;

    unsigned int start=0;
    unsigned int end=len-1;
    unsigned int i,j;
    for(i=start; i < len; i++) 
        if(!isspace(str[i])) 
            break;
            
    for(j=end; j >= 0; j--) 
        if(!isspace(str[j])) 
            break;
    start=i;
    end=j;

    RtString *stripped_str = init_RtString(NULL);
    stripped_str->string= malloc_substring(str, start, end);
    stripped_str->length= end-start;

    RtObject *strobj = init_RtObject(STRING_TYPE);
    strobj->data.String=stripped_str;

    return strobj;
}

/**
 * DESCRIPTION:
 * Built in function for returning the first occurence of a sub string
 * If no substring is found, -1 is returned
 * 
*/
static RtObject *builtin_str_find(RtObject *target, RtObject **args, int arg_count) {
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 1);
    char *str1 = target->data.String->string;
    char *str2 = args[0]->data.String->string;
    RtObject *res = init_RtObject(NUMBER_TYPE);
    char *occurence = strstr(str1, str2);
    double index;
    if(occurence) index = (occurence - str1) / sizeof(char) + 1;
    else index = -1;
    res->data.Number = init_RtNumber(index);
    return res;
}

/**
 * DESCRIPTION:
 * Built in function for checking if a string is an alpha numeric
*/
static RtObject *builtin_str_isalnum(RtObject *target, RtObject **args, int arg_count) {
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 0);
    (void)args;
    bool ans = true;
    StrBoolPred(ans, target->data.String->string, target->data.String->length, isalnum);
    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(ans);
    return res;
}

/**
 * DESCRIPTION:
 * Built in function for checking if a string only contains digits
*/
static RtObject *builtin_str_isnumeric(RtObject *target, RtObject **args, int arg_count) {
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 0);
    (void)args;
    bool ans = true;
    StrBoolPred(ans, target->data.String->string, target->data.String->length, isdigit);
    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(ans);
    return res;
}

/**
 * DESCRIPTION:
 * Built in function for checking if a string is an alpha numeric
*/
static RtObject *builtin_str_isalph(RtObject *target, RtObject **args, int arg_count) {
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 0);
    (void)args;
    bool ans = true;
    StrBoolPred(ans, target->data.String->string, target->data.String->length, isalpha);
    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(ans);
    return res;
}

/**
 * DESCRIPTION:
 * Built in function for checking if a string contains only whitespace
*/
static RtObject *builtin_str_isspace(RtObject *target, RtObject **args, int arg_count) {
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 0);
    (void)args;
    bool ans = true;
    StrBoolPred(ans, target->data.String->string, target->data.String->length, isspace);
    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(ans);
    return res;
}

/**
 * DESCRIPTION:
 * Built in function for checking if a string contains only upper case characters
*/
static RtObject *builtin_str_isupper(RtObject *target, RtObject **args, int arg_count) {
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 0);
    (void)args;
    bool ans = true;
    StrBoolPred(ans, target->data.String->string, target->data.String->length, isupper);
    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(ans);
    return res;
}

/**
 * DESCRIPTION:
 * Built in function for checking if a string contains only lower case characters
*/
static RtObject *builtin_str_islower(RtObject *target, RtObject **args, int arg_count) {
    // temp
    assert(target->type == STRING_TYPE);
    assert(arg_count == 0);
    (void)args;
    bool ans = true;
    StrBoolPred(ans, target->data.String->string, target->data.String->length, islower);
    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(ans);
    return res;
}


