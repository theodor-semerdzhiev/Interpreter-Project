#pragma once

#include "generics/hashset.h"
#include <stdio.h>
#include "parser.h"

typedef enum OpCode
{

    // Pushes some constant value onto the stack
    LOAD_CONST,

    // Pushes value mapped to name to stack
    LOAD_VAR,

    // Takes the top element on the stack and sets it to the second element on the stack
    // Note, It sets the pointer to the object it self
    MUTATE_VAR,

    // Takes current object on top of the stack and creates a mapping between it and a variable name
    // Pops top object on the stack
    CREATE_VAR,

    // Pops the top n things on the stack and pushes a list object on the stack
    CREATE_LIST,

    // Pops the top n things on the stack and pushes a set object on the stack
    CREATE_SET,

    // Pops the top n pairs of elements on the stack and pushes a map object on the stack
    // Each pair is a key value pair
    CREATE_MAP,

    // Takes current object on top of the stack and gets the associated attribute (via a identifier)
    // Pops object and pushes atribute object
    LOAD_ATTRIBUTE,

    // Takes current object on the top of the stack
    // Uses it to fetch that index from second object on the stack
    LOAD_INDEX, // stack[n-1][stack[n]]

    // Takes current object on the top of the stack and,
    // Jumps the program counter to a specific location (associated with that object),
    // Storing the return address on the stack
    // Creates a call frame and the required closures, adds it to call stack
    // Arguments
    FUNCTION_CALL,

    // Pushes function object onto stack, used for nameless functions
    // Closure variables should be loaded on the stack first,
    // and will be fetched depending on the number of closures
    CREATE_FUNCTION,

    // Jumps the program counter to fixed index in bytecode
    ABSOLUTE_JUMP,

    // Jumps the program counter to a position relative to its current one (value can be -/+)
    OFFSET_JUMP,

    // Pops element from call stack and returns to that byte code location
    // Pops element from stack and pushes that on the parent call frame
    FUNCTION_RETURN,

    // Same as above, but does not pop the current stack, instead it simply adds UNDEFINED object to parent stack
    FUNCTION_RETURN_UNDEFINED,

    // Terminates program, exit code is fetched from the top of the stack
    EXIT_PROGRAM,
    /*
    Jumps the program counter to a position relative to its current one
    if popped value from stack evals to true
    */
    OFFSET_JUMP_IF_TRUE,

    /* If popped value from stack evals to false */
    OFFSET_JUMP_IF_FALSE,

    /* Pops computation stack */
    POP_STACK,

    /* Deference a variable name to its value, and sets it to the previous mapping, if one is available */
    DEREF_VAR,

    // MATH OPERATORS

    /*
    All these operations take the top 2 elements of the stack, uses it in the computation,
    then pops the 2 values, and pushes the computed value
    */
    ADD_VARS_OP,           // stack [n] + stack [n-1]
    SUB_VARS_OP,           // stack [n] - stack [n-1]
    MULT_VARS_OP,          // stack [n] * stack [n-1]
    DIV_VARS_OP,           // stack [n] / stack [n-1]
    MOD_VARS_OP,           // stack [n] % stack [n-1]
    BITWISE_VARS_AND_OP,   // stack [n] & stack [n-1]
    BITWISE_VARS_OR_OP,    // stack [n] | stack [n-1]
    BITWISE_XOR_VARS_OP,   // stack [n] ^ stack [n-1]
    SHIFT_LEFT_VARS_OP,    // stack [n] << stack [n-1]
    SHIFT_RIGHT_VARS_OP,   // stack [n] >> stack [n-1]
    GREATER_THAN_VARS_OP,  // stack [n] > stack [n-1]
    GREATER_EQUAL_VARS_OP, // stack [n] >= stack [n-1]
    LESSER_THAN_VARS_OP,   // stack [n] > stack [n-1]
    LESSER_EQUAL_VARS_OP,  // stack [n] >= stack [n-1]
    EQUAL_TO_VARS_OP,      // stack [n] == stack [n-1]
    LOGICAL_AND_VARS_OP,   // stack [n] && stack [n-1]
    LOGICAL_OR_VARS_OP,    // stack [n] || stack [n-1]

    /*
    Takes the top element of the stack,
    negates it, popping the stack,
    while adding the new value */
    LOGICAL_NOT_VARS_OP, // ! stack [n]

} OpCode;

// /* Final Runtime ready ByteCode */

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

// Forward declaration
typedef struct RtObject RtObject;
typedef struct ByteCodeList ByteCodeList;

// Generic object for all variables
typedef struct RtObject
{
    RtType type;

    union RtObject_data
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
            ByteCodeList *body;

            char **args;
            int arg_count;
        } Function;

        // Object *obj;
        // Function *func;
        // List *list;
        // HashMap *map;
        // HashSet *set;
    } data;

    // what objects this object references
    // used in garbage collection
    RtObject **refs;
    unsigned int ref_count;

} RtObject;

/* Struct representing a single Bytecode instruction */
typedef struct ByteCode
{
    OpCode op_code;

    union bytecode_data
    {
        struct OFFSET_JUMP_IF_FALSE
        {
            int offset;
        } OFFSET_JUMP_IF_FALSE;

        struct OFFSET_JUMP_IF_TRUE
        {
            int offset;
        } OFFSET_JUMP_IF_TRUE;

        /* Used any sort of program counter jump */
        struct OFFSET_JUMP
        {
            int offset;
        } OFFSET_JUMP;

        struct {
            char* var;
        } DEREF_VAR;

        struct LOAD_CONST
        {
            RtObject *constant;
        } LOAD_CONST;

        struct CREATE_LIST
        {
            int list_length;
        } CREATE_LIST;

        struct CREATE_SET
        {
            int set_size;
        } CREATE_SET;

        struct CREATE_MAP
        {
            // the number of key value pairs
            // So if size is 4, it will pop 8 elements from the stack
            int map_size;
        } CREATE_MAP;

        struct LOAD_VAR
        {
            char *variable;
            int str_length;
        } LOAD_VAR;

        struct CREATE_VAR
        {
            char *new_var_name;
            int str_length;
        } CREATE_VAR;

        struct LOAD_ATTR
        {
            char *attribute_name;
            int str_length;
        } LOAD_ATTR;

        struct CLOSURES
        {
            char **closure_vars;
            int closure_count;
        } CLOSURE;

        struct FUNCTION_CALL
        {
            int arg_count;
        } FUNCTION_CALL;

        struct CREATE_FUNCTION
        {
            RtObject *function;

            char **closures;
            int closure_count;
        } CREATE_FUNCTION;

    } data;

} ByteCode;

/* General struct for a program */
typedef struct ByteCodeList
{
    ByteCode **code;
    int pg_length;

    int malloc_len;
} ByteCodeList;

/* Free Variable Algorithm */
GenericSet *collect_free_vars(AST_List *body);
GenericSet *collect_free_vars_ast_node(AST_node *node);

RtObject *init_RtObject(RtType type);
ByteCodeList *init_ByteCodeList();
ByteCode *init_ByteCode(OpCode code);

ByteCodeList *concat_bytecode_lists(ByteCodeList *lhs, ByteCodeList *rhs);
ByteCodeList *compile_exps_sequence(ExpressionNode **exps, int exps_length);
ByteCodeList *compile_expression_component(ExpressionComponent *cm);
ByteCode *compile_func_declaration(AST_node *function);
ByteCodeList *compile_conditional_chain(AST_node *node);
ByteCodeList *compiled_while_loop(AST_node *node);
ByteCodeList *compile_expression(ExpressionNode *root);
ByteCodeList *compile_code_body(AST_List *body, bool is_global_scope);

void free_ByteCodeList(ByteCodeList *list);
void free_ByteCode(ByteCode *bytecode);

void deconstruct_RtObject(RtObject *obj, int offset);
void deconstruct_bytecode(ByteCodeList *bytecode, int offset);

void free_RtObject(RtObject *obj);
void free_ByteCodeList(ByteCodeList *list);
void free_ByteCode(ByteCode *bytecode);
