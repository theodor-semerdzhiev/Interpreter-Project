#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "rtobjects.h"
#include "../compiler/compiler.h"
#include "../generics/utilities.h"
#include "rtfunc.h"
#include "gc.h"

/**
 * This file contains the implementation of runtime objects, and the corresponding operations on them
 */

static RtObject *multiply_obj_by_multiplier(double multiplier, RtObject *obj);
static void print_offset(int offset);
static char *repeat_string(int repeated, const char *str, int strlen);

/* Helper for printing offset */
static void print_offset(int offset)
{
    for (int i = 0; i < offset; i++)
        printf("        ");
}

__attribute__((warn_unused_result))
/**
 * Helper for creating a repeated string
 */
static char *
repeat_string(int repeated, const char *str, int strlen)
{
    char *string = malloc(sizeof(char) * (repeated * strlen + 1));
    if (!string)
        return NULL;

    string[0] = '\0';

    for (int i = 0; i < repeated; i++)
    {
        strcat(string, str);
        string[strlen * (i + 1)] = '\0';
    }
    return string;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Constructor for Runtime object
 * Function types are set to non built in by default
 */
RtObject *
init_RtObject(RtType type)
{
    RtObject *obj = malloc(sizeof(RtObject));
    if (!obj)
        return NULL;
    obj->type = type;
    obj->mark = false;
    return obj;
}

/**
 * Helper function for getting name of the object's type
 */
const char *rtobj_type_toString(const RtObject *obj)
{
    switch (obj->type)
    {
    case UNDEFINED_TYPE:
        return "Undefined";
    case NULL_TYPE:
        return "Null";
    case NUMBER_TYPE:
        return "Number";
    case STRING_TYPE:
        return "String";
    case CLASS_TYPE:
        return "Object";
    case FUNCTION_TYPE:
        return "Function";
    case LIST_TYPE:
        return "List";
    case HASHMAP_TYPE:
        return "Map";
    case HASHSET_TYPE:
        return "Set";
    }
}

__attribute__((warn_unused_result))
/**
 * Converts a string into its corresponding string representation
 * String is allocated on the heap
 */
char *
rtobj_toString(const RtObject *obj)
{
    switch (obj->type)
    {
    case UNDEFINED_TYPE:
        return cpy_string("undefined");

    case NULL_TYPE:
        return cpy_string("null");

    case NUMBER_TYPE:
    {

        char buffer[65];
        snprintf(buffer, sizeof(buffer), "%lf", obj->data.Number.number);
        return cpy_string(buffer);
    }

    case STRING_TYPE:
        return cpy_string(obj->data.String.string);

    case FUNCTION_TYPE:
    {
        return rtfunc_toString(obj->data.Func);
    }

    case CLASS_TYPE:
    {
        // TODO
        return NULL;
    }

    case LIST_TYPE:
    {
        return rtlist_toString(obj->data.List);
    }

    case HASHMAP_TYPE:
    {
        return rtmap_toString(obj->data.Map);
    }

    case HASHSET_TYPE:
    {
        return NULL;
    }
    }
}

__attribute__((warn_unused_result))
/**
 * This function is a helper for applying multiplication on a runtime object and creating a new object
 * For Functions, Bytecode is never copied, and therefor will never be freed
 */
static RtObject *
multiply_obj_by_multiplier(double multiplier, RtObject *obj)
{
    switch (obj->type)
    {
    case NUMBER_TYPE:
    {
        RtObject *result = init_RtObject(NUMBER_TYPE);
        if (!result)
            return NULL;
        result->data.Number.number = multiplier * obj->data.Number.number;
        return result;
    }

    case NULL_TYPE:
    {
        RtObject *result = init_RtObject(NUMBER_TYPE);
        if (!result)
            return NULL;
        result->data.Number.number = 0;
        return result;
    }

    case STRING_TYPE:
    {
        RtObject *result = init_RtObject(STRING_TYPE);
        if (!result)
            return NULL;
        char *multiplicand = obj->data.String.string;
        int multiplicand_len = obj->data.String.string_length;
        result->data.String.string =
            repeat_string((int)multiplier, multiplicand, multiplicand_len);
        result->data.String.string_length = multiplicand_len * multiplier;
        return result;
    }

    case LIST_TYPE:
    {
        // TODO
        return NULL;
    }

    default:
    {
        // invalid type
        printf("Cannot multiply number and %s\n", rtobj_type_toString(obj));
        return NULL;
    }
    }
}

__attribute__((warn_unused_result))
/**
 * Defines multiplication between runtime objects
 */
RtObject *
multiply_objs(RtObject *obj1, RtObject *obj2)
{
    switch (obj1->type)
    {
    // number * obj2
    case NUMBER_TYPE:
    {
        return multiply_obj_by_multiplier(obj1->data.Number.number, obj2);
    }

    // " ... " * obj2
    case STRING_TYPE:
    {
        switch (obj2->type)
        {
        case NUMBER_TYPE:
        {
            return multiply_obj_by_multiplier(obj2->data.Number.number, obj1);
        }

        default:
            printf("Cannot multiply string and %s\n", rtobj_type_toString(obj2));
            return NULL;
        }
    }
    // null * obj2
    case NULL_TYPE:
    {
        return multiply_obj_by_multiplier(0, obj2);
    }

    default:
    {
        printf("Cannot multiply %s by %s\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        break;
    }
    }
    return NULL;
}

__attribute__((warn_unused_result))
/**
 * Defines addition between runtime objects
 */
RtObject *
add_objs(RtObject *obj1, RtObject *obj2)
{
    assert(obj1 && obj2);
    switch (obj1->type)
    {
    case NUMBER_TYPE:
    {
        switch (obj2->type)
        {
        case NUMBER_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            result->data.Number.number = obj1->data.Number.number + obj2->data.Number.number;
            return result;
        }

        case NULL_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            result->data.Number.number = obj1->data.Number.number;
            return result;
        }

        default:
        {
            printf("Cannot perform addition on Number and %s\n", rtobj_type_toString(obj2));
            break;
        }
        }
        break;
    }

    case STRING_TYPE:
    {
        char *adder1 = obj1->data.String.string;
        int length1 = obj1->data.String.string_length;
        switch (obj2->type)
        {
        case STRING_TYPE:
        {
            RtObject *result = init_RtObject(STRING_TYPE);
            if (!result)
                return NULL;
            char *adder2 = obj2->data.String.string;
            int length2 = obj2->data.String.string_length;
            result->data.String.string = concat_strings(adder1, adder2);
            result->data.String.string_length = length1 + length2;
            return result;
        }

        default:
            printf("Cannot perform addition on String and %s\n", rtobj_type_toString(obj2));
            break;
        }
        break;
    }

    case NULL_TYPE:
    {
        switch (obj2->type)
        {

        case NUMBER_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            result->data.Number.number = obj2->data.Number.number;
            return result;
        }

        case NULL_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            result->data.Number.number = 0;
            return result;
        }

        default:
            printf("Cannot perform addtion for Null and %s\n", rtobj_type_toString(obj1));
            return NULL;
        }
        break;
    }

    case LIST_TYPE:
    {
        switch (obj2->type)
        {
        case LIST_TYPE:
        {
            // TODO
            return NULL;
        }

        default:
        {
            printf("Cannot add List and %s\n", rtobj_type_toString(obj2));
            return NULL;
        }
        }
        break;
    }

    case HASHSET_TYPE:
    {
        switch (obj2->type)
        {
        case HASHSET_TYPE:
        {
            // todo: set addition
            return NULL;
        }
        default:
            printf("Cannot add Map and %s\n", rtobj_type_toString(obj2));
            return NULL;
        }
        break;
    }

    case FUNCTION_TYPE:
    case UNDEFINED_TYPE:
    case HASHMAP_TYPE:
    case CLASS_TYPE:
    default:
        return NULL;
    }
    return NULL;
}

__attribute__((warn_unused_result))
/**
 * Defines substraction for runtime object
 */
RtObject *
substract_objs(RtObject *obj1, RtObject *obj2)
{
    switch (obj1->type)
    {
    case NUMBER_TYPE:
    {

        switch (obj2->type)
        {
        case NUMBER_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            result->data.Number.number = obj1->data.Number.number - obj2->data.Number.number;
            return result;
        }

        case NULL_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            result->data.Number.number = obj1->data.Number.number;
            ;
            return result;
        }

        default:
            printf("Cannot perform substraction on Number and %s.\n", rtobj_type_toString(obj2));
            return NULL;
        }
    }

    case NULL_TYPE:
    {
        switch (obj2->type)
        {
        case NUMBER_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            result->data.Number.number = -1 * obj2->data.Number.number;
            return result;
        }

        case NULL_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            result->data.Number.number = 0;
            return result;
        }

        default:
            printf("Cannot perform substraction on Number and %s.\n", rtobj_type_toString(obj2));
            return NULL;
        }
    }

    case HASHSET_TYPE:
    {
        switch (obj2->type)
        {
        case HASHSET_TYPE:
        {
            // todo
            return NULL;
        }

        default:
        {
            printf("Cannot perform substraction on Set and %s.\n", rtobj_type_toString(obj2));
            break;
        }
        }
    }

    case STRING_TYPE:
    case LIST_TYPE:
    case CLASS_TYPE:
    case HASHMAP_TYPE:
    case FUNCTION_TYPE:
    case UNDEFINED_TYPE:
        printf("Cannot perform substraction on types %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        break;
    }
    return NULL;
}

__attribute__((warn_unused_result))
/**
 * Defines division for runtime objects
 */
RtObject *
divide_objs(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = obj1->data.Number.number / obj2->data.Number.number;
        return obj;
    }
    else
    {
        printf("Cannot perform division on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 * Defines modulus for runtime objects
 */
RtObject *
modulus_objs(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        double x = obj1->data.Number.number;
        double y = obj2->data.Number.number;
        obj->data.Number.number = x - ((int)(x / y) * y);
        return obj;
    }
    else
    {
        printf("Cannot perform modulus on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

/**
 * DESCRIPTION:
 * Performs exponentiation on 2 runtime objects
 *
 * PARAMS:
 * base: base to be exponentiated
 * exponent: self explanatory
 *
 */
RtObject *exponentiate_obj(RtObject *base, RtObject *exponent)
{
    if (base->type == NUMBER_TYPE && exponent->type == NUMBER_TYPE)
    {
        RtObject *result = init_RtObject(NUMBER_TYPE);
        result->data.Number.number = pow(base->data.Number.number, exponent->data.Number.number);
        return result;
    }
    else
    {
        printf("Cannot exponentiate %s with %s.\n", rtobj_type_toString(base), rtobj_type_toString(exponent));
        return init_RtObject(UNDEFINED_TYPE);
    }
}

__attribute__((warn_unused_result))
/**
 * Defines bitwise AND for runtime objects
 */
RtObject *
bitwise_and_objs(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = ((int)obj1->data.Number.number) & ((int)obj2->data.Number.number);
        return obj;
    }
    else
    {
        printf("Cannot perform bitwise AND on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 * Defines bitwise OR for runtime objects
 */
RtObject *
bitwise_or_objs(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = ((int)obj1->data.Number.number) | ((int)obj2->data.Number.number);
        return obj;
    }
    else
    {
        printf("Cannot perform bitwise OR on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 * Defines bitwise XOR for runtime objects
 */
RtObject *
bitwise_xor_objs(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = ((int)obj1->data.Number.number) ^ ((int)obj2->data.Number.number);
        return obj;
    }
    else
    {
        printf("Cannot perform bitwise XOR on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 * Defines SHIFT LEFT (<<) for runtime objects
 */
RtObject *
shift_left_objs(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = ((int)obj1->data.Number.number) << ((int)obj2->data.Number.number);
        return obj;
    }
    else
    {
        printf("Cannot perform left shift on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 * Defines SHIFT RIGHT (>>) for runtime objects
 */
RtObject *
shift_right_objs(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = ((int)obj1->data.Number.number) >> ((int)obj2->data.Number.number);
        return obj;
    }
    else
    {
        printf("Cannot perform left right on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 *  Defines a greater than operation (i.e wether obj1 > obj2)
 */
RtObject *
greater_than_op(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = obj1->data.Number.number > obj2->data.Number.number;
        return obj;
    }
    else if (obj1->type == STRING_TYPE && obj2->type == STRING_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = strcmp(obj1->data.String.string, obj2->data.String.string) > 0;
        return obj;
    }
    else
    {
        printf("Cannot perform greater than on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 *  Defines a greater equal  operation (i.e wether obj1 >= obj2)
 */
RtObject *
greater_equal_op(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = obj1->data.Number.number >= obj2->data.Number.number;
        return obj;
    }
    else if (obj1->type == STRING_TYPE && obj2->type == STRING_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = strcmp(obj1->data.String.string, obj2->data.String.string) >= 0;
        return obj;
    }
    else
    {
        printf("Cannot perform greater than or equal on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 *  Defines a lesser than operation (i.e wether obj1 > obj2)
 */
RtObject *
lesser_than_op(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = obj1->data.Number.number < obj2->data.Number.number;
        return obj;
    }
    else if (obj1->type == STRING_TYPE && obj2->type == STRING_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = strcmp(obj1->data.String.string, obj2->data.String.string) < 0;
        return obj;
    }
    else
    {
        printf("Cannot perform lesser than on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 *  Defines a lesser equal operation (i.e wether obj1 >= obj2)
 */
RtObject *
lesser_equal_op(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = obj1->data.Number.number <= obj2->data.Number.number;
        return obj;
    }
    else if (obj1->type == STRING_TYPE && obj2->type == STRING_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = strcmp(obj1->data.String.string, obj2->data.String.string) <= 0;
        return obj;
    }
    else
    {
        printf("Cannot perform lesser than or equal on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 *  Defines a lesser equal operation (i.e wether obj1 >= obj2)
 */
RtObject *
equal_op(RtObject *obj1, RtObject *obj2)
{
    if (obj1->type == NUMBER_TYPE && obj2->type == NUMBER_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = obj1->data.Number.number == obj2->data.Number.number;
        return obj;
    }
    else if (obj1->type == STRING_TYPE && obj2->type == STRING_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        obj->data.Number.number = strings_equal(obj1->data.String.string, obj2->data.String.string);
        return obj;
    }
    else
    {
        printf("Cannot perform lesser than or equal on %s and %s.\n", rtobj_type_toString(obj1), rtobj_type_toString(obj2));
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 * Defines logical and on runtime objects
 */
RtObject *
logical_and_op(RtObject *obj1, RtObject *obj2)
{
    RtObject *result = init_RtObject(NUMBER_TYPE);
    result->data.Number.number = eval_obj(obj1) && eval_obj(obj2);
    return result;
}

__attribute__((warn_unused_result))
/**
 * Defines logical and on runtime objects
 */
RtObject *
logical_or_op(RtObject *obj1, RtObject *obj2)
{
    RtObject *result = init_RtObject(NUMBER_TYPE);
    result->data.Number.number = eval_obj(obj1) || eval_obj(obj2);
    return result;
}

__attribute__((warn_unused_result))
/**
 * Performs logical not on object, in place
 */
RtObject *
logical_not_op(RtObject *target)
{
    if (target->type == NUMBER_TYPE)
    {
        target->data.Number.number = !target->data.Number.number;
    }
    else
    {
        printf("Cannot perform logical NOT on %s\n", rtobj_type_toString(target));
    }

    return target;
}

/**
 * Wether object evals to true or false
 */
bool eval_obj(const RtObject *obj)
{
    switch (obj->type)
    {
    case UNDEFINED_TYPE:
        return false;
    case NULL_TYPE:
        return false;
    case NUMBER_TYPE:
        return ((int)obj->data.Number.number) ? true : false;
    case STRING_TYPE:
        return obj->data.String.string_length ? true : false;
    case FUNCTION_TYPE:
        return true;
    case CLASS_TYPE:
        // TODO
        return false;
    case LIST_TYPE:
        // TODO
        return obj->data.List->length > 0;
    case HASHMAP_TYPE:
        // TODO
        return false;
    case HASHSET_TYPE:
        // TODO
        return false;
    }
}

/**
 * DESCRIPTION:
 * Hashes runtime object
 */
unsigned int rtobj_hash(const RtObject *obj)
{
    switch (obj->type)
    {

    case NUMBER_TYPE:
        return murmurHashUInt(obj->data.Number.number);
    case STRING_TYPE:
        return djb2_string_hash(obj->data.String.string);
    case FUNCTION_TYPE:
        return obj->data.Func->is_builtin ? hash_pointer((void *)obj->data.Func->func_data.built_in.func) : hash_pointer((void *)obj->data.Func->func_data.user_func.body);
    case CLASS_TYPE:
        // TODO
        return false;

    case UNDEFINED_TYPE:
    case NULL_TYPE:
    case LIST_TYPE:
    case HASHMAP_TYPE:
    case HASHSET_TYPE:
        // NOT HASHABLE
        // TODO, SHOULD RAISE EXCEPTION
        return 0;
    }
}

/**
 * DESCRIPTION:
 * Checks if obj1 and obj2 are equivalent
 */
bool rtobj_equal(const RtObject *obj1, const RtObject *obj2)
{
    assert(obj1 && obj2);
    if (obj1 == obj2)
        return true;

    if (obj1->type != obj2->type)
        return false;

    switch (obj1->type)
    {
    case UNDEFINED_TYPE:
        return true;
    case NULL_TYPE:
    {
        return true;
    }

    case NUMBER_TYPE:
    {
        return (obj2->type == NUMBER_TYPE && obj2->data.Number.number == obj2->data.Number.number);
    }

    case STRING_TYPE:
    {
        return (obj2->type == STRING_TYPE && strings_equal(obj1->data.String.string, obj2->data.String.string));
    }

    case FUNCTION_TYPE:
    {
        // functions are compare by reference
        if (obj2->data.Func->is_builtin != obj1->data.Func->is_builtin)
        {
            return false;
        }
        return obj2->data.Func->is_builtin ? obj2->data.Func->func_data.built_in.func == obj1->data.Func->func_data.built_in.func : obj2->data.Func->func_data.user_func.body == obj1->data.Func->func_data.user_func.body;
    }

    case LIST_TYPE:
    {
        return rtlist_equals(obj1->data.List, obj2->data.List, false);
    }

    case HASHMAP_TYPE:
    {
        return rtmap_equal(obj1->data.Map, obj2->data.Map);
    }

    case HASHSET_TYPE:
    {
        // TODO
        return false;
    }

    case CLASS_TYPE:
    {
        // TODO
        return false;
    }
    }
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Creates a new shallow copy of a Runtime Object,
 * Out references are malloced
 *
 * NOTE: For Function Type, a shallow copy is always peformed, since there is only one function instance
 *
 */
RtObject *
rtobj_shallow_cpy(const RtObject *obj)
{
    RtObject *cpy = init_RtObject(obj->type);
    if (!cpy)
        return NULL;

    switch (obj->type)
    {
    case NUMBER_TYPE:
    {
        cpy->data.Number.number = obj->data.Number.number;
        return cpy;
    }

    case STRING_TYPE:
    {
        cpy->data.String.string = cpy_string(obj->data.String.string);
        cpy->data.String.string_length = obj->data.String.string_length;
        return cpy;
    }

    case NULL_TYPE:
    {
        cpy->data.Number.number = 0;
        return cpy;
    }

    case UNDEFINED_TYPE:
    {
        return cpy;
    }

    case FUNCTION_TYPE:
    {
        // mutate_func_data(cpy, obj, true);
        // performs a deep copy
        cpy->data.Func = rtfunc_cpy(obj->data.Func, true);
        return cpy;
    }

    case LIST_TYPE:
    {
        cpy->data.List = rtlist_cpy(obj->data.List, false);
        return cpy;
    }

    case HASHMAP_TYPE:
    {

        cpy->data.Map = rtmap_cpy(obj->data.Map, false, false);
        return cpy;
    }

    case HASHSET_TYPE:
    {

        // TODO
        return NULL;
    }

    case CLASS_TYPE:
    {

        // TODO
        return NULL;
    }

    default:
        return NULL;
    }
}

__attribute__((warn_unused_result))
/**
 * Creates a new deep copy of a Runtime Object,
 * NOTE: For Function Type, a shallow copy is always peformed, since there is only one function instance
 */
RtObject *
rtobj_deep_cpy(const RtObject *obj)
{
    RtObject *cpy = init_RtObject(obj->type);
    if (!cpy)
        return NULL;

    switch (obj->type)
    {
    case NUMBER_TYPE:
    {
        cpy->data.Number.number = obj->data.Number.number;
        return cpy;
    }

    case STRING_TYPE:
    {
        cpy->data.String.string = cpy_string(obj->data.String.string);
        cpy->data.String.string_length = obj->data.String.string_length;
        return cpy;
    }

    case NULL_TYPE:
    {
        cpy->data.Number.number = 0;
        return cpy;
    }

    case UNDEFINED_TYPE:
    {
        return cpy;
    }

    case FUNCTION_TYPE:
    {
        mutate_func_data(cpy, obj, true);
        return cpy;
    }

    case LIST_TYPE:
    {
        cpy->data.List = rtlist_cpy(obj->data.List, true);
        return cpy;
    }

    case HASHMAP_TYPE:
    {

        cpy->data.Map = rtmap_cpy(obj->data.Map, true, true);
        return cpy;
    }

    case HASHSET_TYPE:
    {

        // TODO
        return NULL;
    }

    case CLASS_TYPE:
    {

        // TODO
        return NULL;
    }
    }
}

/**
 * DESCRIPTION:
 * Performs a mutation on the target, with the new value
 *
 * PARAMS:
 * deepcpy: wether target's data should deep copied or shallow copied from new_value
 */
RtObject *rtobj_mutate(RtObject *target, const RtObject *new_value, bool deepcpy)
{
    assert(target && new_value);
    rtobj_free_data(target, false);

    switch (new_value->type)
    {
    case NUMBER_TYPE:
    {
        target->data.Number.number = new_value->data.Number.number;
        break;
    }

    case STRING_TYPE:
    {
        target->data.String.string =
            deepcpy ? cpy_string(new_value->data.String.string) : new_value->data.String.string;

        target->data.String.string_length = new_value->data.String.string_length;
        break;
    }

    case NULL_TYPE:
    {
        target->data.Number.number = 0;
        break;
    }

    case UNDEFINED_TYPE:
    {
        break;
    }

    case FUNCTION_TYPE:
    {
        mutate_func_data(target, new_value, deepcpy);
        break;
    }

    case LIST_TYPE:
    {
        target->data.List = rtlist_cpy(new_value->data.List, deepcpy);
        break;
    }

    case HASHMAP_TYPE:
    {

        target->data.Map = rtmap_cpy(new_value->data.Map, deepcpy, deepcpy);
        break;
    }

    case HASHSET_TYPE:
    {

        // TODO
        return NULL;
    }

    case CLASS_TYPE:
    {

        // TODO
        return NULL;
    }

    default:
        return NULL;
    }

    target->type = new_value->type;

    return target;
}

/**
 * DESCRIPTION:
 * Takes index from element. The latter can be a list, set, map, etc..
 *
 *
 * NOTE:
 * Returns NULL if obj is not indexable
 * PARAMS:
 * obj: object to index from
 * index: index
 */
RtObject *rtobj_getindex(RtObject *obj, RtObject *index)
{
    assert(obj && index);

    switch (obj->type)
    {
    case NUMBER_TYPE:
    case STRING_TYPE:
    case NULL_TYPE:
    case UNDEFINED_TYPE:
    case FUNCTION_TYPE:
    {
        // TODO SHOULD THROW ERROR
        return NULL;
    }

    case LIST_TYPE:
    {
        if (index->type != NUMBER_TYPE)
        {
            printf("Index must be a number type\n");
            assert(false);
            return NULL;
        }
        long i = (long)index->data.Number.number;
        if ((size_t)i >= obj->data.List->length)
        {
            printf("Index out of bounds, cannot take index %ld of list of length %zu\n", i, obj->data.List->length);
        }

        return rtlist_get(obj->data.List, i);
    }

    case HASHMAP_TYPE:
    {
        RtObject *tmp = rtmap_get(obj->data.Map, index);

        // element is not found
        if (!tmp)
        {
            char *obj_str = rtobj_toString(index);
            // TODO SHOULD RAISE EXCEPTION
            printf("%s could not be found as a key in map\n", obj_str);
            return NULL;
        }
        return tmp;
    }

    case HASHSET_TYPE:
    {

        // TODO
        return NULL;
    }

    case CLASS_TYPE:
    {

        // TODO
        return NULL;
    }
    }
}

/**
 * DESCRIPTION:
 * Gets an object references, this heavily used by the GC.
 *
 * If object does not have any refs, a empty, single size array is created, populated with a NULL terminator
 *
 * NOTE:
 * Make sure to free the returned pointer after use
 */
RtObject **rtobj_getrefs(const RtObject *obj)
{
    assert(obj);
    switch (obj->type)
    {
    case NULL_TYPE:
    case UNDEFINED_TYPE:
    case NUMBER_TYPE:
    case STRING_TYPE:
    {
        RtObject **tmp = malloc(sizeof(RtObject *));
        tmp[0] = NULL;
        return tmp;
    }

    case LIST_TYPE:
    {
        RtObject **refs = rtlist_getrefs(obj->data.List);
        return refs;
    }

    case FUNCTION_TYPE:
    {
        RtObject **refs = rtfunc_getrefs(obj->data.Func);
        return refs;
    }

    case HASHMAP_TYPE:
    {
        RtObject **refs = rtmap_getrefs(obj->data.Map, true, true);
        return refs;
    }

    case CLASS_TYPE:
    {
        RtObject **refs = rtmap_getrefs(obj->data.Class->attrs_table, true, true);
        return refs;
    }

    case HASHSET_TYPE:
    {
        // TODO
        return NULL;
    }
    }
}

/**
 *  Frees objects associated data, but not object itself
 * */
void rtobj_free_data(RtObject *obj, bool free_immutable)
{
    if (!obj)
        return;

    switch (obj->type)
    {
    case UNDEFINED_TYPE:
    case NULL_TYPE:
    case NUMBER_TYPE:
        break;
    case STRING_TYPE:
        free(obj->data.String.string);
        break;
    case CLASS_TYPE:
    {
        if (free_immutable)
        {
            // TODO
        }
        break;
    }
    case FUNCTION_TYPE:
    {
        free_func_data(obj, free_immutable);
        break;
    }

    case LIST_TYPE:
    {
        // TODO
        rtlist_free(obj->data.List);
        break;
    }
    case HASHMAP_TYPE:
    {
        rtmap_free(obj->data.Map, false, false, free_immutable);
        break;
    }
    case HASHSET_TYPE:
    {
        // TODO
        break;
    }
    }
}

/**
 * Frees Runtime object, functions and objects are immutable
 * free_immutable: if true, it will metadata associated with the object
 */
void rtobj_free(RtObject *obj, bool free_immutable)
{
    if (!obj)
        return;
    rtobj_free_data(obj, free_immutable);
    free(obj);
}

/* Prints out Runtime Object */
void rtobj_deconstruct(RtObject *obj, int offset)
{
    if (!obj)
    {
        return;
    }

    print_offset(offset);

    switch (obj->type)
    {
    case NUMBER_TYPE:
        printf(" %f \n", obj->data.Number.number);
        break;
    case STRING_TYPE:
        printf(" \"%s\" \n", obj->data.String.string);
        break;
    case NULL_TYPE:
        printf(" NULL \n");
        break;
    case FUNCTION_TYPE:
    {
        printf("BuiltIn: %s\n", obj->data.Func->is_builtin ? "true" : "false");
        print_offset(offset);

        // prints out name
        if (obj->data.Func->is_builtin)
        {
            printf("Name: %s\n", obj->data.Func->func_data.built_in.func->builtin_name);
            print_offset(offset);
        }
        else
        {
            printf("Name: %s\n", obj->data.Func->func_data.user_func.func_name);
            print_offset(offset);
            // prints closures
            printf("Closures:");
            for (unsigned int i = 0; i < obj->data.Func->func_data.user_func.closure_count; i++)
            {
                if (i + 1 == obj->data.Func->func_data.user_func.closure_count)
                {
                    printf(" %s ", obj->data.Func->func_data.user_func.closures[i]);
                }
                else
                {
                    printf(" %s ,", obj->data.Func->func_data.user_func.closures[i]);
                }
            }
            printf("\n");
            print_offset(offset);
            // prints arguments
            printf("Args: ");
            for (unsigned int i = 0; i < obj->data.Func->func_data.user_func.arg_count; i++)
            {
                if (i + 1 == obj->data.Func->func_data.user_func.arg_count)
                {
                    printf(" %s ", obj->data.Func->func_data.user_func.args[i]);
                }
                else
                {
                    printf(" %s, ", obj->data.Func->func_data.user_func.args[i]);
                }
            }
            printf("\n");
            print_offset(offset);
            printf("Body:\n");
            deconstruct_bytecode(obj->data.Func->func_data.user_func.body, offset + 1);
        }
        break;
    }
    case LIST_TYPE:
    case CLASS_TYPE:
    case HASHMAP_TYPE:
    case HASHSET_TYPE:
        printf(" Not Implemented \n");

    default:
        break;
    }
}