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
#include "string.h"
#include "rttype.h"

/**
 * This file contains the implementation of runtime objects, and the corresponding operations on them
 */

static RtObject *multiply_obj_by_multiplier(double multiplier, const RtObject *obj);
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

/**
 * DESCRIPTION:
 * Helper for setting number data for runtime object
 * 
 * PARAMS:
 * obj: rt object to mutate
 * num: number
*/
RtObject *set_rtobj_number_data(RtObject *obj, long double num) {
    assert(obj);
    obj->type = NUMBER_TYPE;
    obj->data.Number = init_RtNumber(num);
    return obj;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Constructor for Runtime object
 * Function types are set to non built in by default
 * 
 * NOTE:
 * If the type is NULL_TYPE or UNDEFINED_TYPE, then a GC Flag is malloced
 */
RtObject *
init_RtObject(RtType type)
{
    RtObject *obj = malloc(sizeof(RtObject));
    if (!obj) {
        MallocError();
        return NULL;
    }
    obj->type = type;

    if(type == NULL_TYPE) {
        obj->data.GCFlag_NULL_TYPE = malloc(sizeof(bool));
        *obj->data.GCFlag_NULL_TYPE = false;
    } else if(type == UNDEFINED_TYPE) {
        obj->data.GCFlag_UNDEFINED_TYPE = malloc(sizeof(bool));
        *obj->data.GCFlag_UNDEFINED_TYPE = false;
    }

    return obj;
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
        snprintf(buffer, sizeof(buffer), "%Lf", obj->data.Number->number);
        return cpy_string(buffer);
    }

    case STRING_TYPE:
        return cpy_string(obj->data.String->string);

    case FUNCTION_TYPE:
    {
        return rtfunc_toString(obj->data.Func);
    }

    case CLASS_TYPE:
    {
        return rtclass_toString(obj->data.Class);
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
multiply_obj_by_multiplier(double multiplier, const RtObject *obj)
{
    switch (obj->type)
    {
    case NUMBER_TYPE:
    {
        RtObject *result = init_RtObject(NUMBER_TYPE);
        if (!result) return NULL;
        set_rtobj_number_data(result, multiplier * obj->data.Number->number);
        return result;
    }

    case NULL_TYPE:
    {
        RtObject *result = init_RtObject(NUMBER_TYPE);
        if (!result) return NULL;
        set_rtobj_number_data(result, 0);
        return result;
    }

    case STRING_TYPE:
    {
        RtObject *result = init_RtObject(STRING_TYPE);
        if (!result)
            return NULL;

        unsigned int multiplicand_len = obj->data.String->length;
        char *multiplicand = obj->data.String->string;
        char* new_str = malloc(sizeof(char)*(multiplicand_len*multiplier+1));

        for(unsigned int i=0; i < (unsigned int)multiplier; i++) {
            for(unsigned int j=0; j < multiplicand_len; j++) {
                new_str[multiplicand_len*i+j]=multiplicand[j];
            }
        }
        new_str[multiplicand_len*(int)multiplier]='\0';

        result->data.String=init_RtString(NULL);
        result->data.String->string = new_str;
        result->data.String->length = multiplicand_len*multiplier;
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
        printf("Cannot multiply number and %s\n", rtobj_type_toString(obj->type));
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
        return multiply_obj_by_multiplier(obj1->data.Number->number, obj2);
    }

    // " ... " * obj2
    case STRING_TYPE:
    {
        switch (obj2->type)
        {
        case NUMBER_TYPE:
        {
            return multiply_obj_by_multiplier(obj2->data.Number->number, obj1);
        }

        default:
            printf("Cannot multiply string and %s\n", rtobj_type_toString(obj2->type));
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
        printf("Cannot multiply %s by %s\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
            set_rtobj_number_data(result, obj1->data.Number->number + obj2->data.Number->number);
            return result;
        }

        case NULL_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result) return NULL;
            set_rtobj_number_data(result, obj1->data.Number->number);
            return result;
        }

        default:
        {
            printf("Cannot perform addition on Number and %s\n", 
            rtobj_type_toString(obj2->type));
            break;
        }
        }
        break;
    }

    case STRING_TYPE:
    {
        char *adder1 = obj1->data.String->string;
        int length1 = obj1->data.String->length;
        switch (obj2->type)
        {
        case STRING_TYPE:
        {
            RtObject *result = init_RtObject(STRING_TYPE);
            if (!result)
                return NULL;
            char *adder2 = obj1->data.String->string;
            int length2 = obj1->data.String->length;
            result->data.String=init_RtString(NULL);
            result->data.String->string = concat_strings(adder1, adder2);
            result->data.String->length = length1 + length2;
            return result;
        }

        default:
            printf("Cannot perform addition on String and %s\n", 
            rtobj_type_toString(obj2->type));
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
            set_rtobj_number_data(result, obj2->data.Number->number);
            return result;
        }

        case NULL_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            set_rtobj_number_data(result, 0);
            return result;
        }

        default:
            printf("Cannot perform addtion for Null and %s\n", 
            rtobj_type_toString(obj1->type));
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
            printf("Cannot add List and %s\n", 
            rtobj_type_toString(obj2->type));
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
            printf("Cannot add Map and %s\n", 
            rtobj_type_toString(obj2->type));
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
            set_rtobj_number_data(result, obj1->data.Number->number - obj2->data.Number->number);
            return result;
        }

        case NULL_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            set_rtobj_number_data(result, obj1->data.Number->number);
            
            return result;
        }

        default:
            printf("Cannot perform substraction on Number and %s.\n", 
            rtobj_type_toString(obj2->type));
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
            set_rtobj_number_data(result, -1 * obj2->data.Number->number);
            return result;
        }

        case NULL_TYPE:
        {
            RtObject *result = init_RtObject(NUMBER_TYPE);
            if (!result)
                return NULL;
            set_rtobj_number_data(result, 0);
            return result;
        }

        default:
            printf("Cannot perform substraction on Number and %s.\n", 
            rtobj_type_toString(obj2->type));
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
            printf("Cannot perform substraction on Set and %s.\n", 
            rtobj_type_toString(obj2->type));
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
        printf("Cannot perform substraction on types %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        set_rtobj_number_data(obj, (double)(obj1->data.Number->number / obj2->data.Number->number));
        return obj;
    }
    else
    {
        printf("Cannot perform division on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double x = obj1->data.Number->number;
        double y = obj2->data.Number->number;
        double num = x - ((int)(x / y) * y);
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform modulus on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double num = pow(base->data.Number->number, exponent->data.Number->number);
        set_rtobj_number_data(result, num);
        return result;
    }
    else
    {
        printf("Cannot exponentiate %s with %s.\n", 
        rtobj_type_toString(base->type), 
        rtobj_type_toString(exponent->type));
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
        double num = ((int)obj1->data.Number->number) & ((int)obj2->data.Number->number);
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform bitwise AND on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double num = ((int)obj1->data.Number->number) | ((int)obj2->data.Number->number);
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform bitwise OR on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double num = ((int)obj1->data.Number->number) ^ ((int)obj2->data.Number->number);
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform bitwise XOR on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double num = ((int)obj1->data.Number->number) << ((int)obj2->data.Number->number);
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform left shift on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double num = ((int)obj1->data.Number->number) >> ((int)obj2->data.Number->number);
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform left right on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double num = obj1->data.Number->number > obj2->data.Number->number;
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else if (obj1->type == STRING_TYPE && obj2->type == STRING_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        double num = strcmp(obj1->data.String->string, obj2->data.String->string) > 0;
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform greater than on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double num = obj1->data.Number->number >= obj2->data.Number->number;
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else if (obj1->type == STRING_TYPE && obj2->type == STRING_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        double num = strcmp(obj1->data.String->string, obj2->data.String->string) >= 0;
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform greater than or equal on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double num = obj1->data.Number->number < obj2->data.Number->number;
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else if (obj1->type == STRING_TYPE && obj2->type == STRING_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        double num = strcmp(obj1->data.String->string, obj2->data.String->string) < 0;
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform lesser than on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
        double num = obj1->data.Number->number <= obj2->data.Number->number;
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else if (obj1->type == STRING_TYPE && obj2->type == STRING_TYPE)
    {
        RtObject *obj = init_RtObject(NUMBER_TYPE);
        double num = strcmp(obj1->data.String->string, obj2->data.String->string) <= 0;
        set_rtobj_number_data(obj, num);
        return obj;
    }
    else
    {
        printf("Cannot perform lesser than or equal on %s and %s.\n", 
        rtobj_type_toString(obj1->type), 
        rtobj_type_toString(obj2->type));
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
    RtObject *result = init_RtObject(NUMBER_TYPE);
    double num = (double)rtobj_equal(obj1, obj2);
    set_rtobj_number_data(result, num);
    return result;
}

__attribute__((warn_unused_result))
/**
 * Defines logical and on runtime objects
 */
RtObject *
logical_and_op(RtObject *obj1, RtObject *obj2)
{
    RtObject *result = init_RtObject(NUMBER_TYPE);
    double num = eval_obj(obj1) && eval_obj(obj2);
    set_rtobj_number_data(result, num);
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
    double num = eval_obj(obj1) || eval_obj(obj2);
    set_rtobj_number_data(result, num);
    return result;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Performs logical not on object, in place 
 * 
 * NOTE:
 * target must be a number type
 */
RtObject *
logical_not_op(RtObject *target)
{
    if (target->type == NUMBER_TYPE)
    {
        target->data.Number->number = !target->data.Number->number;
    }
    else
    {
        printf("Cannot perform logical NOT on %s\n", 
        rtobj_type_toString(target->type));
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
        return ((int)obj->data.Number->number) ? true : false;
    case STRING_TYPE:
        return obj->data.String->length ? true : false;
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
        return murmurHashUInt(obj->data.Number->number);
    case STRING_TYPE:
        return djb2_string_hash(obj->data.String->string);
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
 * Hashes the DATA pointer ITSELF associated with the rtobject
*/
unsigned int rtobj_hash_data_ptr(const RtObject *obj) {
    switch (obj->type)
    {
    case NUMBER_TYPE: return hash_pointer(obj->data.Number);
    case STRING_TYPE: return hash_pointer(obj->data.String);
    case FUNCTION_TYPE: return hash_pointer(obj->data.Func);
    case CLASS_TYPE: return hash_pointer(obj->data.Func);
    case UNDEFINED_TYPE: return hash_pointer(obj->data.GCFlag_UNDEFINED_TYPE);
    case NULL_TYPE: return hash_pointer(obj->data.GCFlag_NULL_TYPE);
    case LIST_TYPE: return hash_pointer(obj->data.List);
    case HASHMAP_TYPE: return hash_pointer(obj->data.Map);
    case HASHSET_TYPE:
        // TODO
        return 0;
    }
}

/**
 * DESCRIPTION:
 * Checks if rtobjects have the same type AND the same DATA pointer
*/
bool rtobj_shallow_equal(const RtObject *obj1, const RtObject *obj2) {
    assert(obj1 && obj2);

    if(obj1->type != obj2->type) return false;

    switch (obj1->type)
    {
    case NUMBER_TYPE: return obj1->data.Number == obj2->data.Number;
    case STRING_TYPE: return obj1->data.String == obj2->data.String;
    case FUNCTION_TYPE: return obj1->data.Func == obj2->data.Func;
    case CLASS_TYPE: return obj1->data.Class == obj2->data.Class;
    case UNDEFINED_TYPE: return obj1->data.GCFlag_UNDEFINED_TYPE == obj2->data.GCFlag_UNDEFINED_TYPE;
    case NULL_TYPE: return obj1->data.GCFlag_NULL_TYPE == obj2->data.GCFlag_NULL_TYPE;
    case LIST_TYPE: return obj1->data.List == obj2->data.List;
    case HASHMAP_TYPE: return obj1->data.Map == obj2->data.Map;
    case HASHSET_TYPE:
        // TODO
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
        return obj1->data.Number->number == obj2->data.Number->number;
    }

    case STRING_TYPE:
    {
        return strings_equal(obj1->data.String->string, obj2->data.String->string);
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
        cpy->data.Number = obj->data.Number;
        return cpy;
    }

    case NULL_TYPE:
    {
        // needs to be freed since init_RtObject malloced
        free(cpy->data.GCFlag_NULL_TYPE);
        cpy->data.GCFlag_NULL_TYPE = obj->data.GCFlag_NULL_TYPE;
        return cpy;
    }

    case UNDEFINED_TYPE:
    {
        // needs to be freed since init_RtObject malloced
        free(cpy->data.GCFlag_UNDEFINED_TYPE);
        cpy->data.GCFlag_UNDEFINED_TYPE = obj->data.GCFlag_UNDEFINED_TYPE;
        return cpy;
    }

    case STRING_TYPE:
    {
        cpy->data.String = obj->data.String;
        return cpy;
    }

    case FUNCTION_TYPE:
    {
        cpy->data.Func = obj->data.Func;
        return cpy;
    }

    case LIST_TYPE:
    {
        cpy->data.List = obj->data.List;
        return cpy;
    }

    case HASHMAP_TYPE:
    {

        cpy->data.Map = obj->data.Map;
        return cpy;
    }

    case CLASS_TYPE:
    {

        cpy->data.Class = obj->data.Class;
        return cpy;
    }

    case HASHSET_TYPE:
    {

        // TODO
        return NULL;
    }

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
        cpy->data.Number = init_RtNumber(obj->data.Number->number);
        return cpy;
    }

    case STRING_TYPE:
    {
        cpy->data.String = init_RtString(obj->data.String->string);
        return cpy;
    }

    // init_RtObject should have malloced a new bool*
    case NULL_TYPE:
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

        cpy->data.Class = rtclass_cpy(obj->data.Class, true, true);
        return cpy;
    }
    }
}

/**
 * DESCRIPTION:
 * Performs a mutation on the target, with the new value
 *
 * PARAMS:
 * new_val_disposable: wether new_value data is disposable (i.e. will be freed use)
 * 
 */
RtObject *rtobj_mutate(RtObject *target, const RtObject *new_value, bool new_val_disposable)
{
    assert(target && new_value);

    if(target == new_value) {
        return target;
    }

    assert(GC_Registry_has(target));

    /**
     * TODO:
     * THIS LINE IS VERY PROBLEMATIC
    */

    // add_to_GC_registry(rtobj_shallow_cpy(target));

    switch (new_value->type)
    {
    case NUMBER_TYPE:
    {
        // new copy is created each time
        target->data.Number = init_RtNumber(new_value->data.Number->number);
        break;
    }

    case NULL_TYPE:
    {
        target->data.GCFlag_NULL_TYPE = malloc(sizeof(bool));
        *target->data.GCFlag_NULL_TYPE = false;
        break;
    }

    case UNDEFINED_TYPE:
    {
        target->data.GCFlag_UNDEFINED_TYPE = malloc(sizeof(bool));
        *target->data.GCFlag_UNDEFINED_TYPE = false;
        break;
    }

    case STRING_TYPE:
    {
        target->data.String =  new_value->data.String;
        break;
    }

    case FUNCTION_TYPE:
    {
        target->data.Func = new_value->data.Func;
        break;
    }
 
    case LIST_TYPE:
    {
        target->data.List = new_value->data.List;
        break;
    }

    case HASHMAP_TYPE:
    {
        target->data.Map = new_value->data.Map;
        break;
    }

    case HASHSET_TYPE:
    {

        // TODO
        return NULL;
    }

    case CLASS_TYPE:
    {
        target->data.Class =  new_value->data.Class;
        break;
    }

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
        printf("Cannot get index of type %s\n", rtobj_type_toString(obj->type));
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
        long i = (long)index->data.Number->number;
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
 * DESCRIPTION:
 * Gets the GC flag of the data pointer associated with the object depending on its type
*/
bool rtobj_get_GCFlag(const RtObject *obj) {
    switch (obj->type)
    {
    case NULL_TYPE:
        return *obj->data.GCFlag_NULL_TYPE;
    case UNDEFINED_TYPE:
        return *obj->data.GCFlag_UNDEFINED_TYPE;
    case NUMBER_TYPE:
        return obj->data.Number->GCFlag;
    case STRING_TYPE:
        return obj->data.String->GCFlag;
    case LIST_TYPE:
        return obj->data.List->GCFlag;
    case FUNCTION_TYPE:
        return obj->data.Func->GCFlag;
    case HASHMAP_TYPE:
        return obj->data.Map->GCFlag;
    case CLASS_TYPE:
        return obj->data.Class->GCFlag;

    case HASHSET_TYPE:
        // TODO
        return true;
    }
}

/**
 * DESCRIPTION:
 * Sets the GC flag of the associated data to the flag parameter
*/
void rtobj_set_GCFlag(RtObject *obj, bool flag) {
    switch (obj->type)
    {
    case NULL_TYPE:
        *obj->data.GCFlag_NULL_TYPE=flag;
        return;
    case UNDEFINED_TYPE:
        *obj->data.GCFlag_UNDEFINED_TYPE=flag;
        return;
    case NUMBER_TYPE:
        obj->data.Number->GCFlag = flag;
        return;
    case STRING_TYPE:
        obj->data.String->GCFlag = flag;
        return;
    case LIST_TYPE:
        obj->data.List->GCFlag = flag;
        return;
    case FUNCTION_TYPE:
        obj->data.Func->GCFlag = flag;
        return;
    case HASHMAP_TYPE:
        obj->data.Map->GCFlag = flag;
        return;
    case CLASS_TYPE:
        obj->data.Class->GCFlag = flag;
        return;

    case HASHSET_TYPE:
        // TODO
        return;
    }
}

/**
 * DESCRIPTION:
 * Creates a rtobject with the given type and associated data pointer
 * A shallow copy is created
 * PARAMS:
 * type: type of the new rt object
 * data: pointer to set the union value
*/
RtObject *rtobj_init(RtType type, void* data) {
    assert(data);
    RtObject *obj = init_RtObject(type);

    switch (obj->type)
    {
    case NULL_TYPE:
        obj->data.GCFlag_NULL_TYPE= (bool*)data;
        break;
    case UNDEFINED_TYPE:
        obj->data.GCFlag_UNDEFINED_TYPE= (bool*)data;
        break;
    case NUMBER_TYPE:
        obj->data.Number=(RtNumber*)data;
        break;
    case STRING_TYPE:
        obj->data.String=(RtString*)data;
        break;
    case LIST_TYPE:
        obj->data.List=(RtList*)data;
        break;
    case FUNCTION_TYPE:
        obj->data.Func=(RtFunction*)data;
        break;
    case HASHMAP_TYPE:
        obj->data.Map=(RtMap*)data;
        break;
    case CLASS_TYPE:
        obj->data.Class=(RtClass*)data;
        break;

    case HASHSET_TYPE:
        // TODO
        break;
    }
    return obj;
}

/**
 * DESCRIPTION:
 * Gets the data pointer associated with the object
 * 
 * NOTE:
 * If object type does not have an associated data pointer, then this function returns obj
*/
void *rtobj_getdata(const RtObject *obj) {
    assert(obj);
    switch (obj->type)
    {
    case NULL_TYPE:
        return obj->data.GCFlag_NULL_TYPE;
    case UNDEFINED_TYPE:
        return obj->data.GCFlag_UNDEFINED_TYPE;
    case NUMBER_TYPE:
        return obj->data.Number;
    case STRING_TYPE:
        return obj->data.String;
    case LIST_TYPE:
        return obj->data.List;
    case FUNCTION_TYPE:
        return obj->data.Func;
    case CLASS_TYPE:
        return obj->data.Class;
    case HASHMAP_TYPE:
        return obj->data.Map;
    case HASHSET_TYPE:
        // return NULL
        return NULL;
    }
    return NULL;
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
        free(obj->data.GCFlag_UNDEFINED_TYPE);
        break;
    case NULL_TYPE:
        free(obj->data.GCFlag_NULL_TYPE);
        break;
    case NUMBER_TYPE:
        rtnum_free(obj->data.Number);
        break;
    case STRING_TYPE:
        rtstr_free(obj->data.String);
        obj->data.String=NULL;
        break;
    case CLASS_TYPE:
    {
        rtclass_free(obj->data.Class, false, free_immutable);
        obj->data.Class=NULL;
        break;
    }
    case FUNCTION_TYPE:
    {
        free_func_data(obj, free_immutable);
        obj->data.Func=NULL;
        break;
    }

    case LIST_TYPE:
    {
        rtlist_free(obj->data.List, false);
        obj->data.List=NULL;
        break;
    }
    case HASHMAP_TYPE:
    {
        rtmap_free(obj->data.Map, false, false, free_immutable);
        obj->data.Map=NULL;
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
 * DESCRIPTION:
 * Frees Runtime object, functions and objects are immutable
 * 
 * PARAMS:
 * free_immutable: if true, it will metadata associated with the object
 */
void rtobj_free(RtObject *obj, bool free_immutable)
{
    if (!obj)
        return;
    rtobj_free_data(obj, free_immutable);
    free(obj);
}

/**
 * DESCRIPTION:
 * Frees rt object without freeing any object data 
*/

void rtobj_shallow_free(RtObject *obj) {
    if(!obj) {
        return;
    }
    free(obj);
}

/* Prints out Runtime Object */
void rtobj_deconstruct(RtObject *obj, int offset)
{
    if (!obj)
        return;

    print_offset(offset);

    switch (obj->type)
    {
    case NULL_TYPE: {
        printf(" NULL \n");
        break;
    }

    case UNDEFINED_TYPE: {
        printf(" Undefined \n");
        break;
    }
    case NUMBER_TYPE:
        printf(" %Lf \n", obj->data.Number->number);
        break;
    case STRING_TYPE:
        printf(" \"%s\" \n", obj->data.String->string);
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