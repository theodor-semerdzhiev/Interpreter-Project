#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../generics/hashset.h"
#include "../generics/utilities.h"
#include "../parser/parser.h"
#include "../compiler/compiler.h"
#include "../runtime/rtobjects.h"
#include "../runtime/rtfunc.h"

#include "exprsimplifier.h"

/*
This file contains the main logic for bytecode compiler implementation
*/

/** Free Variable Collection algorithm Implementation **/
/*

The code below is responsible for traversing
a code body and recursively capturing free variables
(i.e variables that must be declared outside of said body )

1- Loops across AST list (linked list)
2- For each node, it recursively captures all free variables from its associated expressions, expression components, and/or its AST list
    2.1 - Some variable identifier is located
    2.2 - if it is not present inside the bounded variable set, it must be free variable
    2.3 - If identifier is free variable then its added to the free variable set
    2.4 - For all Function and Object arguments as well as statically defined List, Set and Map constant elements, they will always be added as bounded variables.
    2.5 - When a code body as been traversed, all variables defined in a equal/deeper nesting level, are cleared from the bounded variables set

3- Two sets are used to keep track of free variables and bounded variables
NOTE: Free variable set should never have elements removed from it.
4- At the end, bounded variable set is discarded, and free variable set is returned


*/

/* Used for hashing */
typedef struct FreeVariable
{
    char *varname;
    int nesting_lvl;
} FreeVariable;

static FreeVariable *_malloc_free_var_struct(const char *varname, int nesting_lvl);
static void _free_var_struct(FreeVariable *var);
static bool _are_free_vars_equal_via_name(const FreeVariable *var1, const FreeVariable *var2);
static unsigned _hash_free_vars(const FreeVariable *var);

typedef bool (*NestingLevelFilter)(const FreeVariable *var);
static NestingLevelFilter _filter_bge_nesting_lvl(int nesting_lvl);

static void _collect_free_vars_from_exp(int recursion_lvl, ExpressionNode *root, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _add_sequence_of_exps_free_vars(int recursion_lvl, ExpressionNode **args, int arg_count, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _add_sequence_as_bounded_vars(int recursion_lvl, ExpressionNode **args, int arg_count, GenericSet *bound_var_set);
static void _collect_free_vars_from_exp_component(
    int recursion_lvl,
    ExpressionComponent *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set);

static void _collect_free_vars_var_declaration(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_var_assignment(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_if_conditional(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_else_if_conditional(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_while_loop(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_func_declaration(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_inline_func_declaration(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_obj_declaration(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_from_ast_node(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_from_body(int recursion_lvl, AST_List *body, GenericSet *free_var_set, GenericSet *bound_var_set);

/******************************************/

/* Top Level function for capturing all free variables inside a code body,
returns NULL if malloc error occurred */
GenericSet *collect_free_vars(AST_List *node)
{
    GenericSet *free_var_table = init_GenericSet(
        (bool (*)(const void *, const void *))_are_free_vars_equal_via_name,
        (unsigned int (*)(const void *))_hash_free_vars,
        (void (*)(void *))_free_var_struct);

    if (!free_var_table)
        return NULL;

    GenericSet *bound_var_table = init_GenericSet(
        (bool (*)(const void *, const void *))_are_free_vars_equal_via_name,
        (unsigned int (*)(const void *))_hash_free_vars,
        (void (*)(void *))_free_var_struct);

    if (!bound_var_table)
    {
        GenericSet_free(free_var_table, true);
        return NULL;
    }

    _collect_free_vars_from_body(0, node, free_var_table, bound_var_table);
    GenericSet_free(bound_var_table, true);
    return free_var_table;
}

/**
 * DESCRIPTION:
 * Top Level function for capturing all free variables contained within THE BODY of a AST node
 *
 * This function returns a Set of FreeVariable structs, therefor make sure to cast the void* to FreeVariable
 * when extracting elements from the set
 * */
GenericSet *collect_free_vars_ast_node(AST_node *node)
{
    GenericSet *free_var_table = init_GenericSet(
        (bool (*)(const void *, const void *))_are_free_vars_equal_via_name,
        (unsigned int (*)(const void *))_hash_free_vars,
        (void (*)(void *))_free_var_struct);

    if (!free_var_table)
        return NULL;

    GenericSet *bound_var_table = init_GenericSet(
        (bool (*)(const void *, const void *))_are_free_vars_equal_via_name,
        (unsigned int (*)(const void *))_hash_free_vars,
        (void (*)(void *))_free_var_struct);

    if (!bound_var_table)
    {
        GenericSet_free(free_var_table, true);
        return NULL;
    }

    _collect_free_vars_from_ast_node(0, node, free_var_table, bound_var_table);
    GenericSet_free(bound_var_table, true);
    return free_var_table;
}

/////////////////////////////////////////////////////////////////

/* Initializes var struct used in hashmap */
static FreeVariable *_malloc_free_var_struct(const char *varname, int nesting_lvl)
{
    FreeVariable *var = malloc(sizeof(FreeVariable));
    var->nesting_lvl = nesting_lvl;
    var->varname = malloc_string_cpy(NULL, varname);
    return var;
}

/* Frees Free Variable struct */
static void _free_var_struct(FreeVariable *var)
{
    free(var->varname);
    free(var);
}

/* Compares if two variable structs are equal, used for hashmap */
static bool _are_free_vars_equal_via_name(const FreeVariable *var1, const FreeVariable *var2)
{
    return strcmp(var1->varname, var2->varname) == 0;
}

/* Hash function for free variable struct */
static unsigned _hash_free_vars(const FreeVariable *var)
{
    return djb2_string_hash(var->varname);
}

/* Wrapper function for filtering elements with greater/equal nesting level */
static int _nesting_lvl = 0;
static bool _filter_by_nesting_lvl(const FreeVariable *var) { return var->nesting_lvl >= _nesting_lvl; }
typedef bool (*NestingLevelFilter)(const FreeVariable *var);
static NestingLevelFilter _filter_bge_nesting_lvl(const int nesting_lvl)
{
    _nesting_lvl = nesting_lvl;
    return _filter_by_nesting_lvl;
}

/* Traverse the expression tree and finds free variables */
static void _collect_free_vars_from_exp(int recursion_lvl, ExpressionNode *root, GenericSet *free_var_set, GenericSet *bound_var_set)
{
    /* Bases Cases */
    // In theory, this case should never happen
    if (!root)
        return;

    // Reached a leaf
    if (root->type == VALUE)
    {

        // collects free vars from expression component
        _collect_free_vars_from_exp_component(recursion_lvl, root->component, free_var_set, bound_var_set);
        return;
    }

    _collect_free_vars_from_exp(recursion_lvl + 1, root->LHS, free_var_set, bound_var_set);
    _collect_free_vars_from_exp(recursion_lvl + 1, root->RHS, free_var_set, bound_var_set);
}

/* Takes a list of expressions and adds them to the free_var_set if they are NOT bound */
static void _add_sequence_of_exps_free_vars(int recursion_lvl, ExpressionNode **args, int arg_count, GenericSet *free_var_set, GenericSet *bound_var_set)
{
    for (int i = 0; i < arg_count; i++)
    {
        _collect_free_vars_from_exp(recursion_lvl, args[i], free_var_set, bound_var_set);
    }
}

/* Helper function fpr taking a list of arguments and adds them to the bound_var_set as bounded variables */
static void _add_sequence_as_bounded_vars(int recursion_lvl, ExpressionNode **args, int arg_count, GenericSet *bound_var_set)
{
    for (int i = 0; i < arg_count; i++)
    {
        assert(args[i]->type == VALUE);

        char *arg_name = args[i]->component->meta_data.variable_reference;

        FreeVariable var = {
            arg_name,
            recursion_lvl};

        if (!GenericSet_get(bound_var_set, &var))
        {
            FreeVariable *var = _malloc_free_var_struct(arg_name, recursion_lvl);
            GenericSet_insert(bound_var_set, var, false);
        }
    }
}

/* Collects free variables from expression component */
static void _collect_free_vars_from_exp_component(
    int recursion_lvl,
    ExpressionComponent *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    while (node)
    {
        switch (node->type)
        {
        // literals cannot be free variables
        case STRING_CONSTANT:
        case NUMERIC_CONSTANT:
        case NULL_CONSTANT:
            break;

        case LIST_CONSTANT:
        {
            ExpressionNode **list_elements = node->meta_data.list_const.list_elements;
            int list_length = node->meta_data.list_const.list_length;

            // collects list contents
            _add_sequence_of_exps_free_vars(recursion_lvl, list_elements, list_length, free_var_set, bound_var_set);
            break;
        }

        case HASHMAP_CONSTANT:
        {
            // adds key value pairs
            for (int i = 0; i < node->meta_data.HashMap.size; i++)
            {
                ExpressionNode *key = node->meta_data.HashMap.pairs[i]->key;
                ExpressionNode *value = node->meta_data.HashMap.pairs[i]->value;

                _collect_free_vars_from_exp(recursion_lvl, key, free_var_set, bound_var_set);
                _collect_free_vars_from_exp(recursion_lvl, value, free_var_set, bound_var_set);
            }
            break;
        }
        case HASHSET_CONSTANT:
        {
            ExpressionNode **values = node->meta_data.HashSet.values;
            const int set_size = node->meta_data.HashSet.size;

            // adds contents of hashset
            _add_sequence_of_exps_free_vars(recursion_lvl, values, set_size, free_var_set, bound_var_set);
            break;
        }

        case INLINE_FUNC:
        {
            ExpressionNode **args = node->meta_data.inline_func->ast_data.func_args.func_prototype_args;
            const int arg_count = node->meta_data.inline_func->ast_data.func_args.args_num;

            // adds function arguments as bounded variables
            _add_sequence_as_bounded_vars(recursion_lvl, args, arg_count, bound_var_set);

            _collect_free_vars_from_body(recursion_lvl + 1, node->meta_data.inline_func->body, free_var_set, bound_var_set);
            break;
        }
        case FUNC_CALL:
        {
            ExpressionNode **args = node->meta_data.func_data.func_args;
            const int arg_count = node->meta_data.func_data.args_num;

            // adds sequence of arguments as free variables if not bounded
            _add_sequence_of_exps_free_vars(recursion_lvl, args, arg_count, free_var_set, bound_var_set);
            break;
        }

        case LIST_INDEX:
        {
            _collect_free_vars_from_exp(recursion_lvl, node->meta_data.list_index, free_var_set, bound_var_set);
            break;
        }

        case VARIABLE:
        {
            if (!node->sub_component)
            {
                FreeVariable var;
                var.nesting_lvl = recursion_lvl;
                var.varname = node->meta_data.variable_reference;

                // only adds var to free variable set, if and only if
                // var is not a already in free var set, is not a bound var, or is not a built in identifier
                if (!GenericSet_get(bound_var_set, &var) && !GenericSet_get(free_var_set, &var) && !ident_is_builtin(var.varname))
                {
                    FreeVariable *var_ = _malloc_free_var_struct(var.varname, var.nesting_lvl);
                    GenericSet_insert(free_var_set, var_, false);
                }
            }
            break;
        }

        default:
            break;
        }

        node = node->sub_component;
    }
}

/* Handles case for variable declaration */
static void _collect_free_vars_var_declaration(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{

    assert(node->type == VAR_DECLARATION);

    // collects free variables fro RHS
    _collect_free_vars_from_exp(recursion_lvl, node->ast_data.exp, free_var_set, bound_var_set);

    char *varname = node->identifier.declared_var;

    FreeVariable var;
    var.nesting_lvl = recursion_lvl;
    var.varname = varname;

    // collects free variable for LHS
    if (!GenericSet_get(bound_var_set, &var))
    {
        FreeVariable *var_ = _malloc_free_var_struct(varname, recursion_lvl);
        GenericSet_insert(bound_var_set, var_, false);
    }
}

/* Handles case for variable assignment */
static void _collect_free_vars_var_assignment(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    // finds free variables for LHS
    _collect_free_vars_from_exp_component(
        recursion_lvl,
        node->identifier.expression_component,
        free_var_set,
        bound_var_set);

    // find free variables for RHS
    _collect_free_vars_from_exp(recursion_lvl, node->ast_data.exp, free_var_set, bound_var_set);
}

/* Handles case for if conditional */
static void _collect_free_vars_if_conditional(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    // collects free variables in if expression
    _collect_free_vars_from_exp(recursion_lvl, node->ast_data.exp, free_var_set, bound_var_set);

    // collects free variables in if body
    _collect_free_vars_from_body(recursion_lvl + 1, node->body, free_var_set, bound_var_set);
}

/* Handles case for else if conditional*/
static void _collect_free_vars_else_if_conditional(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    // collects free variables in if expression
    _collect_free_vars_from_exp(recursion_lvl, node->ast_data.exp, free_var_set, bound_var_set);

    // collects free variables in if body
    _collect_free_vars_from_body(recursion_lvl + 1, node->body, free_var_set, bound_var_set);
}

/* Handles case for while loop */
static void _collect_free_vars_while_loop(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    // collects free variable in while expression
    _collect_free_vars_from_exp(recursion_lvl, node->ast_data.exp, free_var_set, bound_var_set);
    // collects free_variables in while body
    _collect_free_vars_from_body(recursion_lvl + 1, node->body, free_var_set, bound_var_set);
}

/* Handles case for function declaration */
static void _collect_free_vars_func_declaration(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    ExpressionNode **args = node->ast_data.func_args.func_prototype_args;
    int arg_count = node->ast_data.func_args.args_num;
    char *func_name = node->identifier.func_name;

    FreeVariable var = {func_name, recursion_lvl};

    // adds function as a bounded variable
    if (!GenericSet_get(bound_var_set, &var))
    {
        FreeVariable *var = _malloc_free_var_struct(func_name, recursion_lvl);
        GenericSet_insert(bound_var_set, var, false);
    }

    // collects bound variables from function arguments
    _add_sequence_as_bounded_vars(recursion_lvl, args, arg_count, bound_var_set);
    // collects free variables from function body
    _collect_free_vars_from_body(recursion_lvl + 1, node->body, free_var_set, bound_var_set);
}

/* Handles case for inline function delclaration */
static void _collect_free_vars_inline_func_declaration(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    ExpressionNode **args = node->ast_data.func_args.func_prototype_args;
    const int arg_count = node->ast_data.func_args.args_num;

    // adds function arguments as bounded variables
    _add_sequence_as_bounded_vars(recursion_lvl, args, arg_count, bound_var_set);

    // collects free variables in function body
    _collect_free_vars_from_body(recursion_lvl + 1, node->body, free_var_set, bound_var_set);
}

/* Handles case for object declarations */
static void _collect_free_vars_obj_declaration(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    ExpressionNode **args = node->ast_data.obj_args.object_prototype_args;
    const int arg_count = node->ast_data.obj_args.args_num;
    char *obj_name = node->identifier.obj_name;

    // adds object arguments as bounded variables
    _add_sequence_as_bounded_vars(recursion_lvl, args, arg_count, bound_var_set);

    FreeVariable var = {obj_name, recursion_lvl};

    // adds object as a bounded variable
    if (!GenericSet_get(bound_var_set, &var))
    {
        FreeVariable *var = _malloc_free_var_struct(obj_name, recursion_lvl);
        GenericSet_insert(bound_var_set, var, false);
    }

    // collects free variables in object body
    _collect_free_vars_from_body(recursion_lvl + 1, node->body, free_var_set, bound_var_set);
}

/* Logic for capturing all free variables within the body of a ast node*/
static void _collect_free_vars_from_ast_node(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    switch (node->type)
    {
    case VAR_DECLARATION:
        _collect_free_vars_var_declaration(recursion_lvl, node, free_var_set, bound_var_set);
        break;

    case VAR_ASSIGNMENT:
        _collect_free_vars_var_assignment(recursion_lvl, node, free_var_set, bound_var_set);
        break;

    case IF_CONDITIONAL:
        _collect_free_vars_if_conditional(recursion_lvl, node, free_var_set, bound_var_set);
        break;
    case ELSE_CONDITIONAL:
        // collects free variables in else body
        _collect_free_vars_from_body(recursion_lvl + 1, node->body, free_var_set, bound_var_set);
        break;
    case ELSE_IF_CONDITIONAL:
        _collect_free_vars_else_if_conditional(recursion_lvl, node, free_var_set, bound_var_set);
        break;
    case WHILE_LOOP:
        _collect_free_vars_while_loop(recursion_lvl, node, free_var_set, bound_var_set);
        break;
    case FUNCTION_DECLARATION:
        _collect_free_vars_func_declaration(recursion_lvl, node, free_var_set, bound_var_set);
        break;

    case RETURN_VAL:
        // collects free variables in return expression
        _collect_free_vars_from_exp(recursion_lvl, node->ast_data.exp, free_var_set, bound_var_set);
        break;

    // Loop termination and continuation does not have expressions
    case LOOP_TERMINATOR:
    case LOOP_CONTINUATION:
        break;

    case EXPRESSION_COMPONENT:
        _collect_free_vars_from_exp_component(
            recursion_lvl,
            node->identifier.expression_component,
            free_var_set,
            bound_var_set);

        break;

    case INLINE_FUNCTION_DECLARATION:
        _collect_free_vars_inline_func_declaration(recursion_lvl, node, free_var_set, bound_var_set);
        break;

    case CLASS_DECLARATION:
        _collect_free_vars_obj_declaration(recursion_lvl, node, free_var_set, bound_var_set);
        break;

    default:
        break;
    }
}

/* Collects free vars contained wihtin the body of a AST node */
static void _collect_free_vars_from_body(
    int recursion_lvl,
    AST_List *body,
    GenericSet *free_var_set,
    GenericSet *bound_var_set)
{
    if (!body)
        return;

    AST_node *ptr = body->head;
    while (ptr)
    {
        _collect_free_vars_from_ast_node(recursion_lvl, ptr, free_var_set, bound_var_set);
        ptr = ptr->next;
    }

    /* Removes all bounded variables defined at equal or deeper nesting level */
    GenericSet_filter(bound_var_set, (bool (*)(void *))_filter_bge_nesting_lvl(recursion_lvl), true);
}

/**
 * Main Implementation for turning AST_List, AST_node and ExpressionComponents into correct bytecode
 *
 *
 *
 * **/

static bool ast_list_has(AST_List *body, enum ast_node_type type);
static ByteCodeList *add_var_derefs(AST_List *body, ByteCodeList *target);
static void add_var_derefs_via_list(ByteCodeList *list);
static bool expression_component_has(ExpressionComponent *cm, enum expression_component_type type);
static void print_offset(int offset);

/* Mallocs ByteCode */
ByteCode *init_ByteCode(OpCode code)
{
    ByteCode *bytecode = malloc(sizeof(ByteCode));
    bytecode->op_code = code;
    return bytecode;
}

/* Adds bytecode instruction to Byte Code list, return the list itself */
ByteCodeList *add_bytecode(ByteCodeList *pg, ByteCode *instr)
{
    pg->code[pg->pg_length] = instr;
    pg->pg_length++;
    if (pg->pg_length == pg->malloc_len)
    {
        pg->malloc_len *= 2;
        ByteCode **new_arr = malloc(sizeof(ByteCode *) * pg->malloc_len);
        for (int i = 0; i < pg->pg_length; i++)
        {
            new_arr[i] = pg->code[i];
        }
        free(pg->code);
        pg->code = new_arr;
    }
    return pg;
}

#define DEFAULT_BYTECODE_LIST_LENGTH 256;

/* Initializes Byte Code list */
ByteCodeList *init_ByteCodeList()
{
    ByteCodeList *list = malloc(sizeof(ByteCodeList));
    list->malloc_len = DEFAULT_BYTECODE_LIST_LENGTH;
    list->pg_length = 0;
    list->code = malloc(sizeof(ByteCode *) * list->malloc_len);
    return list;
}

// Takes both lists, and concatenates them together
// Frees both lists, creates new one
ByteCodeList *concat_bytecode_lists(ByteCodeList *lhs, ByteCodeList *rhs)
{
    if (!lhs && !rhs)
    {
        // returns empty list
        return init_ByteCodeList();
    }

    if (!lhs)
    {
        return rhs;
    }
    else if (!rhs)
    {
        return lhs;
    }

    for (int i = 0; i < rhs->pg_length; i++)
    {
        add_bytecode(lhs, rhs->code[i]);
    }

    free(rhs->code);
    free(rhs);

    return lhs;
}

/* Function compiles sequence of expressions,
such that that when run with a stack machine, it will terminate with
the result of each expression on the stack (i.e tape) */
ByteCodeList *compile_exps_sequence(ExpressionNode **exps, const int exps_length)
{
    ByteCodeList *compiled_exps = NULL;
    for (int i = 0; i < exps_length; i++)
    {
        compiled_exps = concat_bytecode_lists(compiled_exps, compile_expression(exps[i]));
    }

    return compiled_exps;
}

/* Recursively Compiles expression component (expression leaf) */
ByteCodeList *compile_expression_component(ExpressionComponent *cm)
{
    if (!cm)
    {
        return NULL;
    }
    ByteCodeList *list = NULL;

    // recursive case
    if (cm->sub_component)
    {
        list = compile_expression_component(cm->sub_component);

        // case case, reach LHS side of expression Component
    }
    else
    {
        list = init_ByteCodeList();
    }

    assert(list);
    ByteCode *instruction = NULL;

    // TODO
    switch (cm->type)
    {

    // terminal constants
    case NUMERIC_CONSTANT:
    {
        instruction = init_ByteCode(LOAD_CONST);
        RtObject *number_constant = init_RtObject(NUMBER_TYPE);
        number_constant->data.Number = cm->meta_data.numeric_const;
        instruction->data.LOAD_CONST.constant = number_constant;
        break;
    }

    case STRING_CONSTANT:
    {
        instruction = init_ByteCode(LOAD_CONST);
        RtObject *string_constant = init_RtObject(STRING_TYPE);
        string_constant->data.String = init_RtString(cm->meta_data.string_literal);
        instruction->data.LOAD_CONST.constant = string_constant;
        break;
    }

    case LIST_CONSTANT:
    {

        const int list_length = cm->meta_data.list_const.list_length;
        ExpressionNode **elements = cm->meta_data.list_const.list_elements;

        ByteCodeList *compiled_seq = compile_exps_sequence(elements, list_length);
        list = concat_bytecode_lists(list, compiled_seq);

        instruction = init_ByteCode(CREATE_LIST);
        instruction->data.CREATE_LIST.list_length = list_length;

        break;
    }

    case NULL_CONSTANT:
    {

        instruction = init_ByteCode(LOAD_CONST);
        instruction->data.LOAD_CONST.constant = init_RtObject(NULL_TYPE);

        break;
    }

    case HASHMAP_CONSTANT:
    {

        const int key_val_count = cm->meta_data.HashMap.size;
        KeyValue **pairs = cm->meta_data.HashMap.pairs;

        // Loads key value pairs onto the bytecode
        for (int i = 0; i < key_val_count; i++)
        {
            ByteCodeList *key = compile_expression(pairs[i]->key);
            ByteCodeList *val = compile_expression(pairs[i]->value);

            list = concat_bytecode_lists(list, concat_bytecode_lists(key, val));
        }

        instruction = init_ByteCode(CREATE_MAP);
        instruction->data.CREATE_MAP.map_size = key_val_count;

        break;
    }

    case HASHSET_CONSTANT:
    {
        const int set_size = cm->meta_data.list_const.list_length;
        ExpressionNode **elements = cm->meta_data.list_const.list_elements;

        ByteCodeList *compiled_seq = compile_exps_sequence(elements, set_size);
        list = concat_bytecode_lists(list, compiled_seq);

        instruction = init_ByteCode(CREATE_SET);
        instruction->data.CREATE_SET.set_size = set_size;

        break;
    }

        // special cases

    case VARIABLE:
    {
        // if its non terminal variable
        if (cm->sub_component)
        {
            const char *varname = cm->meta_data.variable_reference;
            instruction = init_ByteCode(LOAD_ATTRIBUTE);
            instruction->data.LOAD_ATTR.attribute_name = malloc_string_cpy(NULL, varname);
            instruction->data.LOAD_ATTR.str_length = strlen(varname);

            // if its a variable
        }
        else
        {
            instruction = init_ByteCode(LOAD_VAR);
            instruction->data.LOAD_VAR.variable = malloc_string_cpy(NULL, cm->meta_data.variable_reference);
        }
        break;
    }

    case LIST_INDEX:
    {
        ExpressionNode *exp = cm->meta_data.list_index;
        ByteCodeList *compiled_exp = compile_expression(exp);
        list = concat_bytecode_lists(list, compiled_exp);
        instruction = init_ByteCode(LOAD_INDEX);
        break;
    }

    case FUNC_CALL:
    {
        int arg_count = cm->meta_data.func_data.args_num;
        ExpressionNode **args = cm->meta_data.func_data.func_args;
        ByteCodeList *compiled_args = compile_exps_sequence(args, arg_count);
        list = concat_bytecode_lists(list, compiled_args);
        instruction = init_ByteCode(FUNCTION_CALL);
        instruction->data.FUNCTION_CALL.arg_count = arg_count;
        break;
    }

    // TODO
    case INLINE_FUNC:
    {
        instruction = compile_func_declaration(cm->meta_data.inline_func);
        break;
    }

    default:
        break;
    }

    assert(instruction);
    add_bytecode(list, instruction);

    return list;
}

/**
 * DESCRIPTION:
 * This algorithm is responsible for recursively compiling an expression into a list of bytecodes
 *
 * The algorithm works in the following way:
 * 1- Performs a simplification alg on the root, to simplify the expression if its possible
 * 2- Performs a inorder traversal on expression until it reaches a leaf, i.e the base case
 * 3- Leaf (i.e Expression component) is compiled
 * 4- When both left and right sub trees are compiled, we compile the non leaf node (i.e some operator)
 * 5- If the root is negated, then we also add an extra bytecode to represent that (LOGICAL_NOT_VARS_OP)
 *
 * There is 2 cases that need handled appropriately: LOGICAL_AND and LOGICAL_OR
 * In order to support short circuit evaluation, we must handles these 2 cases
 * Those 2 are handled in the base case
 */
ByteCodeList *compile_expression(ExpressionNode *root)
{
    if (!root)
        return NULL;

    // simplifies root
    root = simplify_expression(root);

    // Base case (Reached a leaf)
    if (root->type == VALUE)
    {
        ByteCodeList *compiled_leaf = compile_expression_component(root->component);
        if (root->negation)
            add_bytecode(compiled_leaf, init_ByteCode(LOGICAL_NOT_VARS_OP));

        return compiled_leaf;
    }

    // Recursive cases
    ByteCodeList *lhs = compile_expression(root->LHS);
    ByteCodeList *rhs = compile_expression(root->RHS);

    ByteCode *operation = NULL;

    switch (root->type)
    {
    case PLUS:
        operation = init_ByteCode(ADD_VARS_OP);
        break;
    case MINUS:
        operation = init_ByteCode(SUB_VARS_OP);
        break;
    case MULT:
        operation = init_ByteCode(MULT_VARS_OP);
        break;
    case DIV:
        operation = init_ByteCode(DIV_VARS_OP);
        break;
    case MOD:
        operation = init_ByteCode(MOD_VARS_OP);
        break;
    case EXPONENT:
        operation = init_ByteCode(EXP_VARS_OP);
        break;
    case BITWISE_AND:
        operation = init_ByteCode(BITWISE_VARS_AND_OP);
        break;
    case BITWISE_OR:
        operation = init_ByteCode(BITWISE_VARS_OR_OP);
        break;
    case BITWISE_XOR:
        operation = init_ByteCode(BITWISE_XOR_VARS_OP);
        break;
    case SHIFT_LEFT:
        operation = init_ByteCode(SHIFT_LEFT_VARS_OP);
        break;
    case SHIFT_RIGHT:
        operation = init_ByteCode(SHIFT_RIGHT_VARS_OP);
        break;
    case GREATER_THAN:
        operation = init_ByteCode(GREATER_THAN_VARS_OP);
        break;
    case GREATER_EQUAL:
        operation = init_ByteCode(GREATER_EQUAL_VARS_OP);
        break;
    case LESSER_THAN:
        operation = init_ByteCode(LESSER_THAN_VARS_OP);
        break;
    case LESSER_EQUAL:
        operation = init_ByteCode(LESSER_EQUAL_VARS_OP);
        break;
    case EQUAL_TO:
        operation = init_ByteCode(EQUAL_TO_VARS_OP);
        break;

        // There 2 are special cases that involve short circuit evaluation

    case LOGICAL_AND:
    {
        operation = init_ByteCode(LOGICAL_AND_VARS_OP);
        break;
    }
    case LOGICAL_OR:
    {
        operation = init_ByteCode(LOGICAL_OR_VARS_OP);
        break;
    }

    // This case should never happen
    case VALUE:
        break;
    }

    // order of arguments important
    ByteCodeList *list = concat_bytecode_lists(lhs, rhs);

    assert(list);
    list = add_bytecode(list, operation);

    // Special Case if entire expression is negated
    if (root->negation)
        add_bytecode(list, init_ByteCode(LOGICAL_NOT_VARS_OP));

    return list;
}

/* Creates Bytecode for CREATE_FUNCTION, used for inline functions */
ByteCode *compile_func_declaration(AST_node *function)
{
    assert(function->type == INLINE_FUNCTION_DECLARATION || function->type == FUNCTION_DECLARATION);

    AST_List *func_body = function->body;
    char *func_name = function->identifier.func_name;
    int arg_count = function->ast_data.func_args.args_num;
    ExpressionNode **args = function->ast_data.func_args.func_prototype_args;

    // fetches free variables
    GenericSet *free_var_set = collect_free_vars_ast_node(function);

    FreeVariable **free_vars = (FreeVariable **)GenericSet_to_list(free_var_set);

    // Initializes function object
    // RtObject *func = init_RtObject(FUNCTION_TYPE);
    RtFunction *func = init_rtfunc(false);
    func->func_data.user_func.body = compile_code_body(func_body, false, false);
    func->func_data.user_func.func_name =
        function->type == FUNCTION_DECLARATION ? malloc_string_cpy(NULL, func_name) : NULL;

    if (!func->func_data.user_func.body)
        func->func_data.user_func.body = init_ByteCodeList();
    func->func_data.user_func.arg_count = arg_count;
    func->func_data.user_func.args = malloc(sizeof(char *) * arg_count);

    // Sets the arguments
    for (int i = 0; i < arg_count; i++)
    {
        assert(args[i]->type == VALUE);
        func->func_data.user_func.args[i] =
            cpy_string(args[i]->component->meta_data.variable_reference);
    }

    func->func_data.user_func.closure_obj = NULL;
    func->func_data.user_func.closure_count = free_var_set->size;
    func->func_data.user_func.closures = malloc(sizeof(char *) * free_var_set->size);

    // Sets closure variables
    for (unsigned int i = 0; i < free_var_set->size; i++)
    {
        func->func_data.user_func.closures[i] = cpy_string(free_vars[i]->varname);
    }

    // Adds function return <=> a return is not present in top scope of function body OR the last AST node is a else block, meaning
    if ((function->body && function->body->length > 0 && !ast_list_has(function->body, RETURN_VAL)))
    {
        add_bytecode(func->func_data.user_func.body, init_ByteCode(FUNCTION_RETURN_UNDEFINED));
    }

    ByteCode *instruction = init_ByteCode(CREATE_FUNCTION);
    RtObject *func_obj = init_RtObject(FUNCTION_TYPE);
    func_obj->data.Func = func;
    instruction->data.CREATE_FUNCTION.function = func_obj;

    // frees memory
    GenericSet_free(free_var_set, true);
    free(free_vars);

    return instruction;
}

/* Compiles conditional control flow */
ByteCodeList *compile_conditional_chain(AST_node *node, bool is_global_scope)
{

    // bases case
    // Reached end of code block or end of conditonal chain
    if (!node || (node->type != IF_CONDITIONAL &&
                  node->type != ELSE_IF_CONDITIONAL &&
                  node->type != ELSE_CONDITIONAL))
    {
        return NULL;
    }

    // Recursive Case

    ByteCodeList *compiled_node = NULL;
    ByteCode *jump_if_false = NULL;

    switch (node->type)
    {
    case ELSE_IF_CONDITIONAL:
    case IF_CONDITIONAL:
    {
        ByteCodeList *compiled_exp = compile_expression(node->ast_data.exp);
        ByteCodeList *compiled_body = compile_code_body(node->body, true, true);

        jump_if_false = init_ByteCode(OFFSET_JUMP_IF_FALSE_POP);
        jump_if_false->data.OFFSET_JUMP_IF_FALSE_POP.offset =
            compiled_body ? compiled_body->pg_length + 1 : 2;

        add_bytecode(compiled_exp, jump_if_false);
        compiled_node = concat_bytecode_lists(compiled_exp, compiled_body);
        break;
    }
    case ELSE_CONDITIONAL:
        compiled_node = compile_code_body(node->body, true, true);
        break;

    default:
        break;
    }

    ByteCodeList *next = NULL;
    // handles edge case where there 2 or more if statements i.e if(..) {...} if(...) {...}
    // They should be treated as two seperate control flows
    if (node->next && node->next->type != IF_CONDITIONAL)
    {
        next = compile_conditional_chain(node->next, is_global_scope);
    }

    // adds offset jump <=> if / else if body does not contain returns, break, or continue
    // if it does contain, then Offset jump becomes redundant
    if (node->type != ELSE_CONDITIONAL &&
        next &&
        !ast_list_has(node->body, RETURN_VAL) &&
        !ast_list_has(node->body, LOOP_CONTINUATION) &&
        !ast_list_has(node->body, LOOP_TERMINATOR))
    {
        ByteCode *instr = init_ByteCode(OFFSET_JUMP);
        instr->data.OFFSET_JUMP.offset = next->pg_length + 1;

        add_bytecode(compiled_node, instr);

        // updates jump offset
        jump_if_false->data.OFFSET_JUMP_IF_FALSE_POP.offset++;
    }

    compiled_node = concat_bytecode_lists(compiled_node, next);
    return compiled_node;
}

ByteCodeList *compiled_while_loop(AST_node *node)
{
    assert(node->type == WHILE_LOOP);

    ByteCodeList *conditional = compile_expression(node->ast_data.exp);
    ByteCodeList *compiled_body = compile_code_body(node->body, false, true);

    // add_var_derefs_via_list(compiled_body);

    // if while body is empty
    if (!compiled_body)
        compiled_body = init_ByteCodeList();

    ByteCode *jump = init_ByteCode(OFFSET_JUMP_IF_FALSE_POP);
    // +2 because we must jump over the offset jump at the end
    jump->data.OFFSET_JUMP_IF_FALSE_POP.offset = compiled_body->pg_length + 2;

    ByteCodeList *loop_code = concat_bytecode_lists(add_bytecode(conditional, jump), compiled_body);

    // resolves jump offsets for loop
    for (int i = 0; i < loop_code->pg_length; i++)
    {
        if (loop_code->code[i]->op_code == OFFSET_JUMP)
        {
            // loop termination
            if (loop_code->code[i]->data.OFFSET_JUMP.offset == INT32_MAX)
            {
                loop_code->code[i]->data.OFFSET_JUMP.offset = loop_code->pg_length - i + 1;
            }
            // loop continuation
            else if (loop_code->code[i]->data.OFFSET_JUMP.offset == -INT32_MAX)
            {
                loop_code->code[i]->data.OFFSET_JUMP.offset = -i;
            }
        }
    }

    // End of while loop should back to beginning ot loop
    ByteCode *code = init_ByteCode(OFFSET_JUMP);
    code->data.OFFSET_JUMP.offset = (loop_code->pg_length * -1);
    add_bytecode(loop_code, code);

    return loop_code;
}

/**
 * DESCRIPTION:
 * Compiles class body into a function, and adds a special bytecode at the end
 */
ByteCode *compile_class_body(AST_node *node)
{
    assert(node->type == CLASS_DECLARATION);
    ExpressionNode **args = node->ast_data.obj_args.object_prototype_args;
    int arg_count = node->ast_data.obj_args.args_num;

    RtFunction *constructor = init_rtfunc(false);
    constructor->func_data.user_func.body = compile_code_body(node->body, false, false);
    constructor->func_data.user_func.func_name = cpy_string(node->identifier.obj_name);

    // sets the arguments
    constructor->func_data.user_func.arg_count = arg_count;
    constructor->func_data.user_func.args = malloc(sizeof(char*) * (arg_count+1));

    for (int i = 0; i < arg_count; i++)
    {
        assert(args[i]->type == VALUE && args[i]->component->type == VARIABLE);
        char *argname = args[i]->component->meta_data.variable_reference;
        constructor->func_data.user_func.args[i] = cpy_string(argname);
    }

    // sets the closured
    GenericSet *free_vars_set = collect_free_vars_ast_node(node);
    FreeVariable **free_vars = (FreeVariable **)GenericSet_to_list(free_vars_set);

    constructor->func_data.user_func.closure_count = free_vars_set->size;
    constructor->func_data.user_func.closure_obj = NULL;
    constructor->func_data.user_func.closures = malloc(sizeof(char*) * free_vars_set->size);

    for (int i = 0; free_vars[i] != NULL; i++)
    {
        constructor->func_data.user_func.closures[i] = cpy_string(free_vars[i]->varname);
    }
    free(free_vars);
    GenericSet_free(free_vars_set, true);

    add_bytecode(constructor->func_data.user_func.body, init_ByteCode(CREATE_OBJECT_RETURN));

    RtObject *constructor_obj = init_RtObject(FUNCTION_TYPE);
    constructor_obj->data.Func = constructor;

    ByteCode *constructor_func = init_ByteCode(CREATE_FUNCTION);
    constructor_func->line_nb = node->line_num;
    constructor_func->data.CREATE_FUNCTION.function = constructor_obj;

    return constructor_func;
}

/* Checks if expression component has node with given type*/
static bool expression_component_has(ExpressionComponent *cm, enum expression_component_type type)
{
    while (cm)
    {
        if (cm->type == type)
            return true;
        cm = cm->sub_component;
    }

    return false;
}

/* Checks if expression component has node with given type*/
static bool ast_list_has(AST_List *body, enum ast_node_type type)
{
    AST_node *node = body->head;
    while (node)
    {
        if (node->type == type)
            return true;
        node = node->next;
    }

    return false;
}

// Returns an array of strings representing the variables created inside a scope
static ByteCodeList *add_var_derefs(AST_List *body, ByteCodeList *target)
{
    AST_node *node = body->head;

    while (node)
    {
        // The rest of ast nodes after is dead code
        if (node->type == LOOP_CONTINUATION ||
            node->type == LOOP_TERMINATOR ||
            node->type == RETURN_VAL)
            break;

        if (node->type == VAR_DECLARATION)
        {
            ByteCode *deref = init_ByteCode(DEREF_VAR);
            deref->data.DEREF_VAR.var = malloc_string_cpy(NULL, node->identifier.declared_var);
            add_bytecode(target, deref);
        }
        node = node->next;
    }

    return target;
}

/*
Similar to above function, but performs the derefs on the bytecode, by matching CREATE_VAR bytecodes
*/
static void add_var_derefs_via_list(ByteCodeList *list)
{
    for (int i = 0; i < list->pg_length; i++)
    {
        if (list->code[i]->op_code == CREATE_VAR)
        {
            ByteCode *deref = init_ByteCode(DEREF_VAR);
            deref->data.DEREF_VAR.var = malloc_string_cpy(NULL, list->code[i]->data.CREATE_VAR.new_var_name);
            add_bytecode(list, deref);
        }
    }
}

/* Compiles a Code Block body,
Param: append_exit_pg
Used to flag if a EXIT_PROGRAM instruction should be appended to body, only used in the global scope */
ByteCodeList *compile_code_body(AST_List *body, bool is_global_scope, bool add_derefs)
{
    if (!body)
        return NULL;

    ByteCodeList *list = NULL;
    AST_node *node = body->head;
    bool as_var_declaration = false;

    while (node)
    {
        switch (node->type)
        {
        case VAR_DECLARATION:
        {
            ByteCodeList *compiled_rhs = compile_expression(node->ast_data.exp);
            list = concat_bytecode_lists(list, compiled_rhs);

            char *varname = node->identifier.declared_var;
            ByteCode *instruction = init_ByteCode(CREATE_VAR);
            instruction->data.CREATE_VAR.new_var_name = malloc_string_cpy(NULL, varname);
            assert(node->access != DOES_NOT_APPLY);
            instruction->data.CREATE_VAR.access = node->access;

            add_bytecode(list, instruction);

            as_var_declaration = true;
            break;
        }

        case VAR_ASSIGNMENT:
        {
            // compiles both sides of assignment
            ByteCodeList *compiled_rhs = compile_expression(node->ast_data.exp);
            ByteCodeList *compiled_lhs = compile_expression_component(node->identifier.expression_component);

            list = concat_bytecode_lists(list, concat_bytecode_lists(compiled_lhs, compiled_rhs));

            ByteCode *instruction = init_ByteCode(MUTATE_VAR);

            add_bytecode(list, instruction);
            break;
        }

        case IF_CONDITIONAL:
        {
            ByteCodeList *conditional_chain = compile_conditional_chain(node, is_global_scope);

            list = concat_bytecode_lists(list, conditional_chain);

            // skips already compiled conditional chain
            while (
                node &&
                node->type != IF_CONDITIONAL &&
                node->type != ELSE_IF_CONDITIONAL &&
                node->type != ELSE_CONDITIONAL)
            {

                node = node->next;
            }

            break;
        }

        case EXPRESSION_COMPONENT:
        {
            // Only compiles, if a function call is present, implying possible variable mutation
            if (expression_component_has(node->identifier.expression_component, FUNC_CALL))
            {
                ByteCodeList *compiled_exp_component = compile_expression_component(node->identifier.expression_component);
                add_bytecode(compiled_exp_component, init_ByteCode(POP_STACK));
                list = concat_bytecode_lists(list, compiled_exp_component);
            }
            break;
        }

        case RETURN_VAL:
        {

            ByteCodeList *return_exp = compile_expression(node->ast_data.exp);
            list = concat_bytecode_lists(list, return_exp);

            ByteCode *return_instruction = NULL;

            if (is_global_scope && !body->parent_block)
            {
                return_instruction = init_ByteCode(EXIT_PROGRAM);
            }
            else if (return_exp)
            {
                return_instruction = init_ByteCode(FUNCTION_RETURN);
            }
            else
            {
                return_instruction = init_ByteCode(FUNCTION_RETURN_UNDEFINED);
            }

            // adds return value if one is not specified
            if (!return_exp && is_global_scope)
            {
                ByteCode *return_val = init_ByteCode(LOAD_CONST);
                return_val->data.LOAD_CONST.constant = init_RtObject(NUMBER_TYPE);
                return_val->data.LOAD_CONST.constant->data.Number = 0;
                add_bytecode(list, return_val);
            }

            add_bytecode(list, return_instruction);
            break;
        }
        // continue
        case LOOP_CONTINUATION:
        {
            ByteCode *jump = init_ByteCode(OFFSET_JUMP);
            jump->data.OFFSET_JUMP.offset = -INT32_MAX;
            if (!list)
                list = init_ByteCodeList();
            add_bytecode(list, jump);
            break;
        }

        // break
        case LOOP_TERMINATOR:
        {
            ByteCode *jump = init_ByteCode(OFFSET_JUMP);
            jump->data.OFFSET_JUMP.offset = INT32_MAX;
            if (!list)
                list = init_ByteCodeList();
            add_bytecode(list, jump);
            break;
        }

        case WHILE_LOOP:
        {
            list = concat_bytecode_lists(list, compiled_while_loop(node));
            break;
        }

        case FUNCTION_DECLARATION:
        case INLINE_FUNCTION_DECLARATION:
        {
            ByteCode *create_func = compile_func_declaration(node);
            ByteCode *create_var = init_ByteCode(CREATE_VAR);
            create_var->data.CREATE_VAR.new_var_name = cpy_string(node->identifier.func_name);
            create_var->data.CREATE_VAR.access = node->access;
            if (!list)
                list = init_ByteCodeList();

            add_bytecode(list, create_func);
            add_bytecode(list, create_var);
            break;
        }

        case CLASS_DECLARATION:
        {
            ByteCode *class_constructor = compile_class_body(node);
            ByteCode *create_var = init_ByteCode(CREATE_VAR);
            create_var->data.CREATE_VAR.new_var_name = cpy_string(node->identifier.func_name);
            create_var->data.CREATE_VAR.access = node->access;

            if (!list)
                list = init_ByteCodeList();

            add_bytecode(list, class_constructor);
            add_bytecode(list, create_var);
            break;
        }

        // these cases should never happen
        case ELSE_CONDITIONAL:
        case ELSE_IF_CONDITIONAL:
            break;
        }

        assert(list);

        // ends compilation, since anything after is dead code
        if (node && (node->type == LOOP_CONTINUATION ||
                     node->type == LOOP_TERMINATOR ||
                     node->type == RETURN_VAL))
        {

            break;
        }

        if (node)
            node = node->next;
    }

    // Adds variable dereferences
    // If and only if, we are not in the global scope, and body does not contain, control flow nodes
    // If it does, then the bytecode will be unreachable
    if (!is_global_scope && add_derefs)
    {
        list = add_var_derefs(body, list);
    }

    // Is only added when in global scope (i.e not nested in a function), and no EXIT_PROGRAM is already present
    if (!ast_list_has(body, RETURN_VAL) && !body->parent_block && is_global_scope)
    {
        ByteCode *return_code_val = init_ByteCode(LOAD_CONST);
        return_code_val->data.LOAD_CONST.constant = init_RtObject(NUMBER_TYPE);
        return_code_val->data.LOAD_CONST.constant->data.Number = 0;
        add_bytecode(list, return_code_val);
        add_bytecode(list, init_ByteCode(EXIT_PROGRAM));
    }

    return list;
}

/* Frees ByteCode struct */
void free_ByteCode(ByteCode *bytecode)
{
    if (!bytecode)
        return;

    switch (bytecode->op_code)
    {
    case LOAD_CONST:
        rtobj_free(bytecode->data.LOAD_CONST.constant, true);
        break;
    case LOAD_VAR:
        free(bytecode->data.LOAD_VAR.variable);
        break;

    case MUTATE_VAR:
        break;

    case DEREF_VAR:
        free(bytecode->data.DEREF_VAR.var);
        break;

    case CREATE_VAR:
        free(bytecode->data.CREATE_VAR.new_var_name);
        break;

    case LOAD_ATTRIBUTE:
        free(bytecode->data.LOAD_ATTR.attribute_name);
        break;

    case CREATE_FUNCTION:
    {
        rtobj_free(bytecode->data.CREATE_FUNCTION.function, true);
        break;
    }

    case POP_STACK:
    case FUNCTION_RETURN_UNDEFINED:
    case EXIT_PROGRAM:
    case FUNCTION_RETURN:
    case CREATE_LIST:
    case CREATE_SET:
    case CREATE_MAP:
    case LOAD_INDEX:
    case FUNCTION_CALL:
    case ABSOLUTE_JUMP:
    case OFFSET_JUMP:
    case OFFSET_JUMP_IF_FALSE_POP:
    case OFFSET_JUMP_IF_TRUE_POP:
    case OFFSET_JUMP_IF_FALSE_NOPOP:
    case OFFSET_JUMP_IF_TRUE_NOPOP:
    case ADD_VARS_OP:
    case SUB_VARS_OP:
    case MULT_VARS_OP:
    case DIV_VARS_OP:
    case MOD_VARS_OP:
    case BITWISE_VARS_AND_OP:
    case BITWISE_VARS_OR_OP:
    case BITWISE_XOR_VARS_OP:
    case SHIFT_LEFT_VARS_OP:
    case SHIFT_RIGHT_VARS_OP:
    case GREATER_THAN_VARS_OP:
    case GREATER_EQUAL_VARS_OP:
    case LESSER_THAN_VARS_OP:
    case LESSER_EQUAL_VARS_OP:
    case EQUAL_TO_VARS_OP:
    case LOGICAL_AND_VARS_OP:
    case LOGICAL_OR_VARS_OP:
    case LOGICAL_NOT_VARS_OP:
        break;

    default:
        break;
    }

    free(bytecode);
}

/* Frees ByteCodeList */
void free_ByteCodeList(ByteCodeList *list)
{
    if (!list)
        return;

    for (int i = 0; i < list->pg_length; i++)
    {
        free_ByteCode(list->code[i]);
    }
    free(list->code);
    free(list);
}

/* Helper for printing offset */
static void print_offset(int offset)
{
    for (int i = 0; i < offset; i++)
    {
        printf("        ");
    }
}

/* Deconstructs bytecode by printing it out */
void deconstruct_bytecode(ByteCodeList *bytecode, int offset)
{
    // Prints offset
    print_offset(offset);

    if (!bytecode)
    {
        printf("Empty\n");
        return;
    }

    for (int i = 0; i < bytecode->pg_length; i++)
    {
        ByteCode *instrc = bytecode->code[i];

        printf("%d      ", i);

        switch (instrc->op_code)
        {
        case DEREF_VAR:
            printf("DEREF_VAR %s\n", instrc->data.DEREF_VAR.var);
            break;
        case LOAD_CONST:
            printf("LOAD_CONST");
            rtobj_deconstruct(instrc->data.LOAD_CONST.constant, offset);
            break;
        case LOAD_VAR:
            printf("LOAD_VAR %s\n", instrc->data.LOAD_VAR.variable);
            break;
        case MUTATE_VAR:
            printf("MUTATE_VAR\n");
            break;
        case CREATE_VAR:
            printf("CREATE_VAR %s \n", instrc->data.CREATE_VAR.new_var_name);
            break;
        case CREATE_LIST:
            printf("CREATE_LIST %d \n", instrc->data.CREATE_LIST.list_length);
            break;
        case CREATE_SET:
            printf("CREATE_SET %d \n", instrc->data.CREATE_SET.set_size);
            break;
        case CREATE_MAP:
            printf("CREATE_MAP %d \n", instrc->data.CREATE_MAP.map_size);
            break;
        case LOAD_ATTRIBUTE:
            printf("LOAD_ATTRIBUTE %s\n", instrc->data.LOAD_ATTR.attribute_name);
            break;
        case LOAD_INDEX:
            printf("LIST_INDEX\n");
            break;
        case FUNCTION_CALL:
            printf("FUNCTION_CALL %d Args \n", instrc->data.FUNCTION_CALL.arg_count);
            break;
        case CREATE_FUNCTION:
        {
            printf("CREATE_FUNCTION\n");
            // print_offset(offset);
            rtobj_deconstruct(instrc->data.CREATE_FUNCTION.function, offset + 1);
            break;
        }

        case CREATE_OBJECT_RETURN:
        {
            printf("CREATE_OBJECT_RETURN\n");
            // TODO
            break;
        }
        case ABSOLUTE_JUMP:
            printf("ABSOLUTE_JUMP\n");
            break;
        case OFFSET_JUMP:
            printf("OFFSET_JUMP: %d offset\n", instrc->data.OFFSET_JUMP.offset);
            break;
        case OFFSET_JUMP_IF_TRUE_POP:
            printf("OFFSET_JUMP_IF_TRUE: %d offset\n", instrc->data.OFFSET_JUMP_IF_TRUE_POP.offset);
            break;
        case OFFSET_JUMP_IF_FALSE_POP:
            printf("OFFSET_JUMP_IF_FALSE: %d offset\n", instrc->data.OFFSET_JUMP_IF_FALSE_POP.offset);
            break;
        case OFFSET_JUMP_IF_TRUE_NOPOP:
            printf("OFFSET_JUMP_IF_TRUE_NOPOP: %d offset\n",
                   instrc->data.OFFSET_JUMP_IF_TRUE_NOPOP.offset);
            break;
        case OFFSET_JUMP_IF_FALSE_NOPOP:
            printf("OFFSET_JUMP_IF_FALSE_NOPOP: %d offset\n",
                   instrc->data.OFFSET_JUMP_IF_FALSE_NOPOP.offset);
            break;
        case FUNCTION_RETURN:
            printf("FUNCTION_RETURN\n");
            break;
        case FUNCTION_RETURN_UNDEFINED:
            printf("FUNCTION_RETURN_UNDEFINED\n");
            break;
        case EXIT_PROGRAM:
            printf("EXIT_PROGRAM\n");
            break;
        case POP_STACK:
            printf("POP_STACK\n");
            break;
        case ADD_VARS_OP:
            printf("ADD_VARS\n");
            break;
        case SUB_VARS_OP:
            printf("SUB_VARS\n");
            break;
        case MULT_VARS_OP:
            printf("MULT_VARS\n");
            break;
        case DIV_VARS_OP:
            printf("DIV_VARS\n");
            break;
        case MOD_VARS_OP:
            printf("MOD_VARS\n");
            break;
        case EXP_VARS_OP:
            printf("EXP_VARS\n");
            break;
        case BITWISE_VARS_AND_OP:
            printf("BITWISE_VARS_AND\n");
            break;
        case BITWISE_VARS_OR_OP:
            printf("BITWISE_VARS_OR\n");
            break;
        case BITWISE_XOR_VARS_OP:
            printf("BITWISE_XOR_VARS\n");
            break;
        case SHIFT_LEFT_VARS_OP:
            printf("SHIFT_LEFT_VARS\n");
            break;
        case SHIFT_RIGHT_VARS_OP:
            printf("SHIFT_RIGHT_VARS\n");
            break;
        case GREATER_THAN_VARS_OP:
            printf("GREATER_THAN_VARS\n");
            break;
        case GREATER_EQUAL_VARS_OP:
            printf("GREATER_EQUAL_VARS\n");
            break;
        case LESSER_THAN_VARS_OP:
            printf("LESSER_THAN_VARS\n");
            break;
        case LESSER_EQUAL_VARS_OP:
            printf("LESSER_EQUAL_VARS\n");
            break;
        case EQUAL_TO_VARS_OP:
            printf("EQUAL_TO_VARS\n");
            break;
        case LOGICAL_AND_VARS_OP:
            printf("LOGICAL_AND_VARS\n");
            break;
        case LOGICAL_OR_VARS_OP:
            printf("LOGICAL_OR_VARS\n");
            break;
        case LOGICAL_NOT_VARS_OP:
            printf("LOGICAL_NOT_VARS\n");
            break;
        }
        if (i + 1 != bytecode->pg_length)
            print_offset(offset);
    }
}
