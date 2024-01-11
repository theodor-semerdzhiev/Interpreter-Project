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
    {
        printf("        ");
    }
}

__attribute__((warn_unused_result))
/**
 * Helper for creating repeated string
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

    if (obj->type == FUNCTION_TYPE)
        obj->data.Function.is_builtin = false;

    obj->outrefs = NULL;
    obj->max_outref = 0;
    obj->outref_count = 0;
    obj->mark = false;
    return obj;
}

/**
 * Helper function for getting name of the object's type
 */
const char *obj_type_toString(const RtObject *obj)
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
    case OBJECT_TYPE:
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
RtObject_to_String(const RtObject *obj)
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
        if (obj->data.Function.is_builtin)
        {
            Builtin *func = obj->data.Function.func_data.built_in.func;
            size_t buffer_length = 65 + strlen(func->builtin_name) + 2 * sizeof(void *) + 1;
            char buffer[buffer_length];
            snprintf(buffer, sizeof(buffer), "%s.func@%p", func->builtin_name, func->builtin_func);

            return cpy_string(buffer);

            // for named functions
        }
        else if (obj->data.Function.func_data.user_func.func_name)
        {
            char *funcname = obj->data.Function.func_data.user_func.func_name;
            size_t buffer_length = 65 + strlen(funcname) + 2 * sizeof(void *) + 1;
            char buffer[buffer_length];
            snprintf(buffer, sizeof(buffer), "%s.func@%p", funcname, obj->data.Function.func_data.built_in.func);

            return cpy_string(buffer);
        }
        else
        {
            size_t buffer_length = 80 + 2 * sizeof(void *) + 1;
            char buffer[buffer_length];
            snprintf(buffer, sizeof(buffer), "func@%p", obj->data.Function.func_data.built_in.func);

            return cpy_string(buffer);
        }
    }

    case OBJECT_TYPE:
    {
        return NULL;
    }

    case LIST_TYPE:
    {
        return NULL;
    }

    case HASHMAP_TYPE:
    {
        return NULL;
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
        printf("Cannot multiply number and %s\n", obj_type_toString(obj));
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
            printf("Cannot multiply string and %s\n", obj_type_toString(obj2));
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
        printf("Cannot multiply %s by %s\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
            printf("Cannot perform addition on Number and %s\n", obj_type_toString(obj2));
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
            result->data.String.string=concat_strings(adder1,adder2);
            result->data.Number.number = length2;
            return result;
        }

        default:
            printf("Cannot perform addition on String and %s\n", obj_type_toString(obj2));
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
            printf("Cannot perform addtion for Null and %s\n", obj_type_toString(obj1));
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
            printf("Cannot add List and %s\n", obj_type_toString(obj2));
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
            printf("Cannot add Map and %s\n", obj_type_toString(obj2));
            return NULL;
        }
        break;
    }

    case FUNCTION_TYPE:
    case UNDEFINED_TYPE:
    case HASHMAP_TYPE:
    case OBJECT_TYPE:
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
            printf("Cannot perform substraction on Number and %s.\n", obj_type_toString(obj2));
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
            printf("Cannot perform substraction on Number and %s.\n", obj_type_toString(obj2));
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
            printf("Cannot perform substraction on Set and %s.\n", obj_type_toString(obj2));
            break;
        }
        }
    }

    case STRING_TYPE:
    case LIST_TYPE:
    case OBJECT_TYPE:
    case HASHMAP_TYPE:
    case FUNCTION_TYPE:
    case UNDEFINED_TYPE:
        printf("Cannot perform substraction on types %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform division on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform modulus on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot exponentiate %s with %s.\n", obj_type_toString(base), obj_type_toString(exponent));
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
        printf("Cannot perform bitwise AND on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform bitwise OR on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform bitwise XOR on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform left shift on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform left right on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform greater than on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform greater than or equal on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform lesser than on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform lesser than or equal on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform lesser than or equal on %s and %s.\n", obj_type_toString(obj1), obj_type_toString(obj2));
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
        printf("Cannot perform logical NOT on %s\n", obj_type_toString(target));
    }

    return target;
}

/**
 * Wether object evals to true or false
 */
bool eval_obj(RtObject *obj)
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
    case OBJECT_TYPE:
        // TODO
        return false;
    case LIST_TYPE:
        // TODO
        return false;
    case HASHMAP_TYPE:
        // TODO
        return false;
    case HASHSET_TYPE:
        // TODO
        return false;
    default:
        return false;
    }
}

__attribute__((warn_unused_result))
/**
 * Creates a new shallow copy of a Runtime Object,
 * Out references are malloced
 * NOTE: For Function Type, a shallow copy is always peformed, since there is only one function instance
 */
RtObject *
shallow_cpy_rtobject(const RtObject *obj)
{
    RtObject *cpy = init_RtObject(obj->type);
    if (!cpy)
        return NULL;

    if (cpy->max_outref > 0)
    {
        // adds refs
        cpy->max_outref = obj->max_outref;
        cpy->outref_count = obj->outref_count;
        cpy->outrefs = malloc(sizeof(RtObject *) * obj->max_outref);
        for (unsigned int i = 0; i < obj->outref_count; i++)
            cpy->outrefs[i] = obj->outrefs[i];
    }

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
        mutate_func_data(cpy, obj, false);
        return cpy;
    }

    case LIST_TYPE:
    {
        // TODO
        return NULL;
    }

    case HASHMAP_TYPE:
    {

        // TODO
        return NULL;
    }

    case HASHSET_TYPE:
    {

        // TODO
        return NULL;
    }

    case OBJECT_TYPE:
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
deep_cpy_rtobject(const RtObject *obj)
{
    RtObject *cpy = init_RtObject(obj->type);
    if (!cpy)
        return NULL;

    // adds refs
    cpy->max_outref = obj->max_outref;
    cpy->outref_count = obj->outref_count;
    cpy->outrefs = malloc(sizeof(RtObject *) * obj->max_outref);

    for (unsigned int i = 0; i < obj->outref_count; i++)
        cpy->outrefs[i] = obj->outrefs[i];

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
        // TODO
        return NULL;
    }

    case HASHMAP_TYPE:
    {

        // TODO
        return NULL;
    }

    case HASHSET_TYPE:
    {

        // TODO
        return NULL;
    }

    case OBJECT_TYPE:
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
RtObject *mutate_obj(RtObject *target, const RtObject *new_value, bool deepcpy)
{
    assert(target && new_value);
    free_RtObject_data(target, false);

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
        // TODO
        return NULL;
    }

    case HASHMAP_TYPE:
    {

        // TODO
        return NULL;
    }

    case HASHSET_TYPE:
    {

        // TODO
        return NULL;
    }

    case OBJECT_TYPE:
    {

        // TODO
        return NULL;
    }

    default:
        return NULL;
    }

    target->type = new_value->type;
    free(target->outrefs);

    if (new_value->outrefs)
    {
        target->outrefs = malloc(sizeof(RtObject *) * new_value->outref_count);
        for (unsigned int i = 0; i < new_value->outref_count; i++)
            target->outrefs[i] = new_value->outrefs[i];

        target->max_outref = new_value->max_outref;
        target->outref_count = new_value->outref_count;
    }
    else
    {
        target->outrefs = NULL;
    }

    return target;
}

/**
 * Adds reference to Object target object
 */
void add_ref(RtObject *target, RtObject *ref)
{
    assert(target && ref);
    if (!target->outrefs)
    {
        target->outrefs = malloc(sizeof(RtObject *) * (DEFAULT_REF + 1));
        target->max_outref = DEFAULT_REF;
        memset(target->outrefs, 0, sizeof(RtObject *) * (DEFAULT_REF + 1));
    }
    assert(target->outref_count < target->max_outref);

    target->outrefs[target->outref_count] = ref;
    target->outref_count++;
    // resizes
    if (target->outref_count == target->max_outref)
    {
        RtObject **new_arr = realloc(target->outrefs, sizeof(RtObject *) * (target->outref_count * 2 + 1));
        target->outrefs = new_arr;
        target->max_outref *= 2;
    }

    target->outrefs[target->outref_count] = NULL;
}

/**
 *  Frees objects associated data, but not object itself
 * */
void free_RtObject_data(RtObject *obj, bool free_immutable)
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
    case OBJECT_TYPE:
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
        break;
    }
    case HASHMAP_TYPE:
    {
        // TODO
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
void free_RtObject(RtObject *obj, bool free_immutable)
{
    if (!obj)
        return;
    free_RtObject_data(obj, free_immutable);
    free(obj->outrefs);
    free(obj);
}

/* Prints out Runtime Object */
void deconstruct_RtObject(RtObject *obj, int offset)
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
        printf("BuiltIn: %s\n", obj->data.Function.is_builtin ? "true" : "false");
        print_offset(offset);

        if (obj->data.Function.is_builtin)
        {
            printf("Name: %s\n", obj->data.Function.func_data.built_in.func->builtin_name);
            print_offset(offset);
        }
        else
        {
            printf("Name: %s\n", obj->data.Function.func_data.user_func.func_name);
            print_offset(offset);
            // prints closures
            printf("Closures:");
            for (int i = 0; i < obj->data.Function.func_data.user_func.closure_count; i++)
            {
                if (i + 1 == obj->data.Function.func_data.user_func.closure_count)
                {
                    printf(" %s ", obj->data.Function.func_data.user_func.closures[i]);
                }
                else
                {
                    printf(" %s ,", obj->data.Function.func_data.user_func.closures[i]);
                }
            }
            printf("\n");
            print_offset(offset);
            // prints arguments
            printf("Args: ");
            for (int i = 0; i < obj->data.Function.func_data.user_func.arg_count; i++)
            {
                if (i + 1 == obj->data.Function.func_data.user_func.arg_count)
                {
                    printf(" %s ", obj->data.Function.func_data.user_func.args[i]);
                }
                else
                {
                    printf(" %s, ", obj->data.Function.func_data.user_func.args[i]);
                }
            }
            printf("\n");
            print_offset(offset);
            printf("Body:\n");
            deconstruct_bytecode(obj->data.Function.func_data.user_func.body, offset + 1);
        }
        break;
    }
    case LIST_TYPE:
    case OBJECT_TYPE:
    case HASHMAP_TYPE:
    case HASHSET_TYPE:
        printf(" Not Implemented \n");

    default:
        break;
    }
}