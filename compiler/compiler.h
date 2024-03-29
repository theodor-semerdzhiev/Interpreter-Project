#pragma once
#include <stdio.h>
#include "../generics/hashset.h"
#include "../parser/parser.h"
#include "../runtime/rtobjects.h"

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

    /**
     * Jumps the program counter to a position relative to its current one
     * if popped value from stack evals to true,
     * It will always pop the stack machine
     */
    OFFSET_JUMP_IF_TRUE_POP,

    /**
     * If popped value from stack evals to false, we perform jump
     * It will always pop the stack machine
     * */
    OFFSET_JUMP_IF_FALSE_POP,

    /**
     * Exactly the same as OFFSET_JUMP_IF_TRUE_POP, but does not pop the stack machine
     * */
    OFFSET_JUMP_IF_FALSE_NOPOP,

    /**
     * Exactly the same as OFFSET_JUMP_IF_TRUE_POP, but does not pop the stack machine
     * */
    OFFSET_JUMP_IF_TRUE_NOPOP,

    /* Pops computation stack */
    POP_STACK,

    /* Deference a variable name to its value, and sets it to the previous mapping, if one is available */
    DEREF_VAR,

    /* Creates a new exception object and pushes it onto the stack machine */
    CREATE_EXCEPTION,

    /* Pushes an exception handler onto the exception stack, this is run when entering try block */
    PUSH_EXCEPTION_HANDLER,

    /* Pops an exception handler from the exception stack, this is run when exiting try block */
    POP_EXCEPTION_HANDLER,

    /* Pops from stack machine and raises exception during runtime */
    RAISE_EXCEPTION,

    /* If currently raised exception does not match exception on stack machine, then raised exception is raised */
    RAISE_EXCEPTION_IF_COMPARE_EXCEPTION_FALSE,

    /* Offsets jump if raised Exception does not match exception on stack machine */
    OFFSET_JUMP_IF_COMPARE_EXCEPTION_FALSE,

    /**
     * Resets currently raised exception
     */
    RESOLVE_RAISED_EXCEPTION,

    /* Used for creating objects, takes all elements in the local lookup table and creates an object, then pushes that onto the stack, then pop the call stack */

    CREATE_OBJECT_RETURN,
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
    EXP_VARS_OP,           // stack [n] ** stack [n-1]
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
    LOGICAL_NOT_VARS_OP // ! stack [n]

} OpCode;

/* Struct representing a single Bytecode instruction */
typedef struct ByteCode
{
    OpCode op_code;
    size_t line_nb; // line number associated with instruction, if not applicable, it will be set to -1

    union
    {
        struct
        {
            int offset;
        } OFFSET_JUMP_IF_FALSE_POP;

        struct
        {
            int offset;
        } OFFSET_JUMP_IF_TRUE_POP;

        struct
        {
            int offset;
        } OFFSET_JUMP_IF_FALSE_NOPOP;

        struct
        {
            int offset;
        } OFFSET_JUMP_IF_TRUE_NOPOP;

        struct
        {
            unsigned int offset;
        } ABSOLUTE_JUMP;

        /* Used any sort of program counter jump */
        struct
        {
            int offset;
        } OFFSET_JUMP;

        struct
        {
            char *var;
        } DEREF_VAR;

        struct
        {
            RtObject *constant;
        } LOAD_CONST;

        struct
        {
            unsigned int list_length;
        } CREATE_LIST;

        struct
        {
            int set_size;
        } CREATE_SET;

        struct
        {
            // the number of key value pairs
            // So if size is 4, it will pop 8 elements from the stack
            int map_size;
        } CREATE_MAP;

        struct
        {
            char *variable;
            int str_length;
        } LOAD_VAR;

        struct
        {
            char *new_var_name;
            AccessModifier access;
        } CREATE_VAR;

        struct
        {
            char *attribute_name;
            int str_length;
        } LOAD_ATTR;

        struct
        {
            char **closure_vars;
            int closure_count;
        } CLOSURE;

        struct
        {
            int arg_count;
        } FUNCTION_CALL;

        struct
        {
            RtObject *function;
        } CREATE_FUNCTION;

        struct
        {
            char *exception;
            AccessModifier access;
        } CREATE_EXCEPTION;

        struct
        {
            int start_of_catch_block;
        } PUSH_EXCEPTION_HANDLER;

        struct
        {
            int offset;
        } OFFSET_JUMP_IF_COMPARE_EXCEPTION_FALSE;

    } data;

} ByteCode;

/* General struct for a program */
typedef struct ByteCodeList
{
    ByteCode **code;
    int pg_length;

    int malloc_len;
} ByteCodeList;

/**
 * DESCRIPTION:
 * This struct is passed to every compilation function
 * It keeps track of all meta data about the AST List, in one place
 */
typedef struct Compiler
{
    char *filename;
} Compiler;

#define compiler_free(compiler) free(compiler->filename); free(compiler);

/* Free Variable Algorithm */
GenericSet *collect_free_vars(AST_List *body);
GenericSet *collect_free_vars_ast_node(AST_node *node);

Compiler *init_Compiler(const char *filename);
ByteCodeList *init_ByteCodeList();
ByteCode *init_ByteCode(OpCode code, size_t line_nb);

ByteCodeList *concat_bytecode_lists(ByteCodeList *lhs, ByteCodeList *rhs);

ByteCodeList *compile_exps_sequence(Compiler *compiler, ExpressionNode **exps, int exps_length);
ByteCodeList *compile_expression_component(Compiler *compiler, ExpressionComponent *cm);
ByteCode *compile_func_declaration(Compiler *compiler, AST_node *function);
ByteCodeList *compile_conditional_chain(Compiler *compiler, AST_node *node, bool is_global_scope);
ByteCodeList *compile_try_catch_chain(Compiler *compiler, AST_node *node, bool is_global_scope, int rec_lvl);
ByteCodeList *compiled_while_loop(Compiler *compiler, AST_node *node, bool is_global_scope);
ByteCodeList *compile_expression(Compiler *compiler, ExpressionNode *root);
ByteCode *compile_class_body(Compiler *compiler, AST_node *node);
ByteCodeList *compile_code_body(Compiler *compiler, AST_List *body, bool is_global_scope, bool add_derefs);
ByteCodeList *compile_raise_exception(Compiler *compiler, AST_node *node);

void free_ByteCodeList(ByteCodeList *list);
void free_ByteCode(ByteCode *bytecode);

void deconstruct_bytecode(ByteCodeList *bytecode, int offset);

void free_ByteCodeList(ByteCodeList *list);
void free_ByteCode(ByteCode *bytecode);
