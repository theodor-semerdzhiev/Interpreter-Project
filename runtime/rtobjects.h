#pragma once
#include <stdbool.h>
#include "builtins.h"

// // Possible runtime types
typedef enum RtType
{
    UNDEFINED_TYPE,
    NULL_TYPE,
    NUMBER_TYPE,
    STRING_TYPE,
    OBJECT_TYPE,
    FUNCTION_TYPE,
    LIST_TYPE,
    HASHMAP_TYPE,
    HASHSET_TYPE,
} RtType;

#define DEFAULT_REF 8

// Forward declaration
typedef struct RtObject RtObject;
typedef struct ByteCodeList ByteCodeList;

// Generic object for all variables
typedef struct RtObject
{
    RtType type;

    union
    {
        // will contain data about runtime object
        struct NumberConstant
        {
            double number;
        } Number;

        struct StringConstant
        {
            char *string;
            int string_length;
        } String;

        struct Function
        {
            bool is_builtin; // flag

            union {
                // regular user defined functions
                struct user_func {
                    ByteCodeList *body;

                    // function arguments
                    char **args;
                    int arg_count;

                    // each closures maps to the closure object
                    char **closures;
                    RtObject **closure_obj;

                    int closure_count;

                    char *func_name;
                } user_func;

                // built in function
                struct built_in {
                    Builtin *func;
                } built_in;
            } func_data;

        } Function;

        // Object *obj;
        // List *list;
        // HashMap *map;
        // HashSet *set;
    } data;

    // what objects this object references
    // used in garbage collection

    /* Objects that this object points to */
    RtObject **outrefs; 
    unsigned int max_outref;
    unsigned int outref_count;


    bool mark; // used for garbage collector
} RtObject;


RtObject *init_RtObject(RtType type);

char *RtObject_to_String(const RtObject *obj);
const char *obj_type_toString(const RtObject *obj);
RtObject *multiply_objs(RtObject *obj1, RtObject *obj2);


RtObject *add_objs(RtObject *obj1, RtObject *obj2);
RtObject *substract_objs(RtObject *obj1, RtObject *obj2);
RtObject *divide_objs(RtObject *obj1, RtObject *obj2);
RtObject *modulus_objs(RtObject *obj1, RtObject *obj2);
RtObject *exponentiate_obj(RtObject *base, RtObject *exponent);
RtObject *bitwise_and_objs(RtObject *obj1, RtObject *obj2);
RtObject *bitwise_or_objs(RtObject *obj1, RtObject *obj2);
RtObject *bitwise_xor_objs(RtObject *obj1, RtObject *obj2);
RtObject *shift_left_objs(RtObject *obj1, RtObject *obj2);
RtObject *shift_right_objs(RtObject *obj1, RtObject *obj2);
RtObject *greater_than_op(RtObject *obj1, RtObject *obj2);
RtObject *greater_equal_op(RtObject *obj1, RtObject *obj2);
RtObject *lesser_than_op(RtObject *obj1, RtObject *obj2);
RtObject *lesser_equal_op(RtObject *obj1, RtObject *obj2);
RtObject *equal_op(RtObject *obj1, RtObject *obj2);
RtObject *logical_and_op(RtObject *obj1, RtObject *obj2);
RtObject *logical_or_op(RtObject *obj1, RtObject *obj2);
RtObject *logical_not_op(RtObject *target);

bool eval_obj(RtObject *obj);
RtObject *shallow_cpy_rtobject(const RtObject *obj);
RtObject *deep_cpy_rtobject(const RtObject *obj);

RtObject *mutate_obj(RtObject *target, const RtObject *new_value, bool deepcpy);

void add_ref(RtObject *target, RtObject *ref);
void free_RtObject_data(RtObject *obj, bool free_immutable);
void free_RtObject(RtObject *obj, bool free_immutable);
void deconstruct_RtObject(RtObject *obj, int offset);