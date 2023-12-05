#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "generics/hashset.h"
#include "parser.h"
#include "generics/utilities.h"
#include "compiler.h"

/* 
This file contains the main logic for bytecode compiler implementation
*/


/** Free Variable Collection algorithm Implementation **/
/* TODO : NEEDS CODE REVIEW

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
static void *_free_var_struct(FreeVariable *var);
static bool _are_free_vars_equal_via_name(const FreeVariable *var1, const FreeVariable *var2);
static unsigned _hash_free_vars(const FreeVariable *var);

typedef bool (*NestingLevelFilter)(const FreeVariable *var);
static NestingLevelFilter _filter_bge_nesting_lvl(const int nesting_lvl);

static void _collect_free_vars_from_exp(int recursion_lvl, const ExpressionNode *root, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _add_sequence_of_exps_free_vars(int recursion_lvl, const ExpressionNode **args, const int arg_count, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _add_sequence_as_bounded_vars(int recursion_lvl, const ExpressionNode **args, const int arg_count, GenericSet *bound_var_set);
static void _collect_free_vars_from_exp_component(
    int recursion_lvl,
    ExpressionComponent *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set);

static void _collect_free_vars_var_declaration(int recursion_lvl,AST_node *node,GenericSet *free_var_set,GenericSet *bound_var_set);
static void _collect_free_vars_var_assignment(int recursion_lvl,AST_node *node,GenericSet *free_var_set,GenericSet *bound_var_set);
static void _collect_free_vars_if_conditional(int recursion_lvl,AST_node *node,GenericSet *free_var_set,GenericSet *bound_var_set);
static void _collect_free_vars_else_if_conditional(int recursion_lvl,AST_node *node,GenericSet *free_var_set,GenericSet *bound_var_set);
static void _collect_free_vars_while_loop(int recursion_lvl, AST_node *node,GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_func_declaration(int recursion_lvl, AST_node *node, GenericSet *free_var_set, GenericSet *bound_var_set);
static void _collect_free_vars_inline_func_declaration(int recursion_lvl,AST_node *node,GenericSet *free_var_set,GenericSet *bound_var_set);
static void _collect_free_vars_obj_declaration(int recursion_lvl,AST_node *node,GenericSet *free_var_set,GenericSet *bound_var_set);

static void _collect_free_vars_from_ast_node(int recursion_lvl,AST_node *node,GenericSet *free_var_set,GenericSet *bound_var_set);
static void _collect_free_vars_from_body(int recursion_lvl,AST_List *body,GenericSet *free_var_set,GenericSet *bound_var_set);

/******************************************/

/* Top Level function for capturing all free variables inside a code body,
returns NULL if malloc error occurred */
GenericSet *collect_free_vars(AST_List *node)
{
    GenericSet *free_var_table = init_GenericSet(
        (bool (*)(void *, void *))_are_free_vars_equal_via_name, 
        (unsigned int (*)(void *))_hash_free_vars, 
        (void (*)(void *))_free_var_struct);

    if (!free_var_table)
        return NULL;

    GenericSet *bound_var_table = init_GenericSet(
        (bool (*)(void *, void *))_are_free_vars_equal_via_name, 
        (unsigned int (*)(void *))_hash_free_vars, 
        (void (*)(void *))_free_var_struct);

    if (!bound_var_table) {
        free_GenericSet(free_var_table, true);
        return NULL;
    }

    _collect_free_vars_from_body(0, node, free_var_table, bound_var_table);
    free_GenericSet(bound_var_table, true);
    return free_var_table;
}

/* Top Level function for capturing all free variables contained within THE BODY of a AST node */
GenericSet *collect_free_vars_ast_node(AST_node *node) 
{
    GenericSet *free_var_table = init_GenericSet(
        (bool (*)(void *, void *))_are_free_vars_equal_via_name, 
        (unsigned int (*)(void *))_hash_free_vars, 
        (void (*)(void *))_free_var_struct);

    if (!free_var_table)
        return NULL;

    GenericSet *bound_var_table = init_GenericSet(
        (bool (*)(void *, void *))_are_free_vars_equal_via_name, 
        (unsigned int (*)(void *))_hash_free_vars, 
        (void (*)(void *))_free_var_struct);

    if (!bound_var_table) {
        free_GenericSet(free_var_table, true);
        return NULL;
    }

    _collect_free_vars_from_ast_node(0,node, free_var_table, bound_var_table);
    free_GenericSet(bound_var_table, true);
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
static void *_free_var_struct(FreeVariable *var)
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

/* Gets left most component from expression component */
static ExpressionComponent *_get_left_most(ExpressionComponent *node)
{
    while (node->sub_component)
    {
        node = node->sub_component;
    }
    return node;
}

/* Traverse the expression tree and finds free variables */
static void _collect_free_vars_from_exp(int recursion_lvl, const ExpressionNode *root, GenericSet *free_var_set, GenericSet *bound_var_set)
{
    /* Bases Cases */
    // In theory, this case should never happen
    if (!root)
        return;

    // Reached a leaf
    if (root->type == VALUE)
    {

        // collects free vars from expression component
        _collect_free_vars_from_exp_component(recursion_lvl,root->component, free_var_set, bound_var_set);
        return;
    }

    _collect_free_vars_from_exp(recursion_lvl+1,root->LHS, free_var_set, bound_var_set);
    _collect_free_vars_from_exp(recursion_lvl+1,root->RHS, free_var_set, bound_var_set);
}

/* Takes a list of expressions and adds them to the free_var_set if they are NOT bound */
static void _add_sequence_of_exps_free_vars(int recursion_lvl, const ExpressionNode **args, const int arg_count, GenericSet *free_var_set, GenericSet *bound_var_set)
{
    for (int i = 0; i < arg_count; i++)
    {
        _collect_free_vars_from_exp(recursion_lvl,args[i],free_var_set,bound_var_set);
    }
}

/* Helper function fpr taking a list of arguments and adds them to the bound_var_set as bounded variables */
static void _add_sequence_as_bounded_vars(int recursion_lvl, const ExpressionNode **args, const int arg_count, GenericSet *bound_var_set)
{
    for (int i = 0; i < arg_count; i++)
    {
        enum expression_token_type t = args[i]->type;
        assert(args[i]->type == VALUE);

        const char *arg_name = args[i]->component->meta_data.variable_reference;

        FreeVariable var = {
            arg_name,
            recursion_lvl};

        if (!set_contains(bound_var_set, &var))
        {
            FreeVariable *var = _malloc_free_var_struct(arg_name, recursion_lvl);
            set_insert(bound_var_set, var);
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
            ExpressionNode ** list_elements = node->meta_data.list_const.list_elements;
            const int list_length = node->meta_data.list_const.list_length;

            // collects list contents 
            _add_sequence_of_exps_free_vars(recursion_lvl, list_elements, list_length, free_var_set, bound_var_set);
            break;
        }

        case HASHMAP_CONSTANT:
        {
            // adds key value pairs
            for(int i=0; i < node->meta_data.HashMap.size; i++) {
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
            ExpressionNode *args = node->meta_data.inline_func->ast_data.func_args.func_prototype_args;
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
            if(!node->sub_component) {
                FreeVariable var = {
                    node->meta_data.variable_reference,
                    recursion_lvl
                };

                if(!set_contains(bound_var_set, &var)) {
                    FreeVariable *var_ = _malloc_free_var_struct(var.varname, var.nesting_lvl);
                    set_insert(free_var_set, var_);
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
    GenericSet *bound_var_set
) {

    assert(node->type == VAR_DECLARATION);

    // collects free variables fro RHS  
    _collect_free_vars_from_exp(recursion_lvl, node->ast_data.exp, free_var_set, bound_var_set);
    char *varname = node->identifier.declared_var;

    FreeVariable var = { varname, recursion_lvl };  

    // collects free variable for LHS
    if (!set_contains(bound_var_set, &var)) {
        FreeVariable *var = _malloc_free_var_struct(varname,recursion_lvl);
        set_insert(bound_var_set, var);
    }
}

/* Handles case for variable assignment */
static void _collect_free_vars_var_assignment(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set
) {
    // finds free variables for LHS
    _collect_free_vars_from_exp_component(
        recursion_lvl,
        node->identifier.expression_component,
        free_var_set,
        bound_var_set
    );

    // find free variables for RHS
    _collect_free_vars_from_exp(recursion_lvl, node->ast_data.exp, free_var_set, bound_var_set);
}

/* Handles case for if conditional */
static void _collect_free_vars_if_conditional(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set
) {
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
    GenericSet *bound_var_set
) {
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
    GenericSet *bound_var_set
) {
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
    GenericSet *bound_var_set
) {
    ExpressionNode **args = node->ast_data.func_args.func_prototype_args;
    const int arg_count = node->ast_data.func_args.args_num;
    const char* func_name = node->identifier.func_name;

    FreeVariable var = {func_name, recursion_lvl};

    // adds function as a bounded variable 
    if(!set_contains(bound_var_set, &var)) {
        FreeVariable *var = _malloc_free_var_struct(func_name,recursion_lvl);
        set_insert(bound_var_set,var);
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
    GenericSet *bound_var_set
) {
    ExpressionNode **args = node->ast_data.func_args.func_prototype_args;
    const int arg_count = node->ast_data.func_args.args_num;

    // adds function arguments as bounded variables
    _add_sequence_as_bounded_vars(recursion_lvl, args, arg_count, bound_var_set);
    
    // collects free variables in function body
    _collect_free_vars_from_body(recursion_lvl+1, node->body, free_var_set, bound_var_set);
}

/* Handles case for object declarations */
static void _collect_free_vars_obj_declaration(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set
) {
    ExpressionNode **args = node->ast_data.obj_args.object_prototype_args;
    const int arg_count = node->ast_data.obj_args.args_num;
    char* obj_name = node->identifier.obj_name;

    // adds object arguments as bounded variables
    _add_sequence_as_bounded_vars(recursion_lvl, args, arg_count, bound_var_set);

    FreeVariable var = {obj_name, recursion_lvl};

    // adds object as a bounded variable 
    if(!set_contains(bound_var_set, &var)) {
        FreeVariable *var = _malloc_free_var_struct(obj_name,recursion_lvl);
        set_insert(bound_var_set,var);
    }

    // collects free variables in object body
    _collect_free_vars_from_body(recursion_lvl+1, node->body, free_var_set, bound_var_set);
}


/* Logic for capturing all free variables within the body of a ast node*/
static void _collect_free_vars_from_ast_node(
    int recursion_lvl,
    AST_node *node,
    GenericSet *free_var_set,
    GenericSet *bound_var_set
) {
    switch (node->type)
    {
    case VAR_DECLARATION:
        _collect_free_vars_var_declaration(recursion_lvl,node,free_var_set,bound_var_set);
        break;

    case VAR_ASSIGNMENT:
        _collect_free_vars_var_assignment(recursion_lvl,node,free_var_set,bound_var_set);
        break;

    case IF_CONDITIONAL:
        _collect_free_vars_var_assignment(recursion_lvl,node,free_var_set,bound_var_set);
        break;
    case ELSE_CONDITIONAL:
        // collects free variables in else body
        _collect_free_vars_from_body(recursion_lvl + 1, node->body, free_var_set, bound_var_set);
        break;
    case ELSE_IF_CONDITIONAL:
        _collect_free_vars_else_if_conditional(recursion_lvl, node, free_var_set, bound_var_set);
        break;
    case WHILE_LOOP:
        _collect_free_vars_while_loop(recursion_lvl,node,free_var_set,bound_var_set);
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
            bound_var_set
        );
        
        break;

    case INLINE_FUNCTION_DECLARATION: 
        _collect_free_vars_inline_func_declaration(recursion_lvl,node,free_var_set,bound_var_set);
        break;

    case OBJECT_DECLARATION: 
        _collect_free_vars_obj_declaration(recursion_lvl,node,free_var_set,bound_var_set);
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

    AST_node *ptr = body->head;
    while (ptr)
    {
        _collect_free_vars_from_ast_node(recursion_lvl,ptr, free_var_set, bound_var_set);
        ptr = ptr->next;
    }

    /* Removes all bounded variables defined at equal or deeper nesting level */
    set_filter_remove(bound_var_set, (bool (*)(void *))_filter_bge_nesting_lvl(recursion_lvl), true);
}

/** Implementation for compiling expressions into bytecode **/

/* Mallocs ByteCode */
ByteCode *init_ByteCode(OpCode code) {
    ByteCode *bytecode = malloc(sizeof(ByteCode));
    bytecode->code = code;
    return bytecode;
}

/* Adds bytecode instruction to Byte Code list */
static void _add_bytecode(ByteCodeList *pg, ByteCode *instr) {
    pg->code[pg->pg_length] = instr;
    pg->pg_length++;
    if(pg->pg_length == pg->malloc_len) {
        pg->malloc_len*=2;
        ByteCode **new_arr = malloc(sizeof(ByteCode*)*pg->malloc_len);
        for(int i=0; i < pg->pg_length; i++) {
            new_arr[i] = pg->code[i];
        }
        free(pg->code);
        pg->code=new_arr;
    }       
}

#define DEFAULT_BYTECODE_LIST_LENGTH 256;

/* Initializes Byte Code list */
ByteCodeList *init_ByteCodeList() {
    ByteCodeList *list = malloc(sizeof(ByteCodeList));
    list->malloc_len = DEFAULT_BYTECODE_LIST_LENGTH;
    list->pg_length=0;
    list->code=malloc(sizeof(ByteCode*) * list->malloc_len);
    for(int i=0; i < list->malloc_len; i++) {
        list->code[i] = NULL;
    }
    return list;
}

/* Mallocs Runtime object, with a name */
RtObject *init_RtObject(RtType type, char* name) {
    RtObject *obj = malloc(sizeof(RtObject));
    obj->type = type;
    obj->refs=NULL;
    obj->ref_count=0;
    obj->name = name;
    if(name) obj->name_len = (int)strlen(name);
    else     obj->name_len = 0;
    return obj;
}

// Takes both lists, and concatenates them together
// Frees both lists, creates new one
ByteCodeList *concat_bytecode_lists(ByteCodeList *lhs, ByteCodeList *rhs) {
    if(!lhs && !rhs) {
        // returns empty list 
        return init_ByteCodeList();
    }

    if(!lhs) {
        return rhs;
    } else if(!rhs) {
        return lhs;
    }

    ByteCodeList *new_list = init_ByteCodeList();
    for(int i =0; i < lhs->pg_length; i++) {
        _add_bytecode(new_list, lhs->code[i]);
    }

    for(int i =0; i < rhs->pg_length; i++) {
        _add_bytecode(new_list, rhs->code[i]);
    }

    free(lhs->code);
    free(lhs);
    free(rhs->code);
    free(rhs);

    return new_list;
}

/* Function compiles sequence of expressions,
such that that when run with a stack machine, it will terminate with 
the result of each expression on the stack (i.e tape) */
ByteCodeList *compile_exps_sequence(ExpressionNode **exps, const int exps_length) {
    ByteCodeList *compiled_exps = NULL;
    for(int i=0; i < exps_length; i++) {
        compiled_exps = concat_bytecode_lists(compiled_exps, compile_expression(exps[i]));
    }

    return compiled_exps;
}

/* Recursively Compiles expression component (expression leaf) */
ByteCodeList *compile_expression_component(ExpressionComponent *cm) {
    if(!cm) {
        return NULL;
    } 
    ByteCodeList *list = NULL;
    
    // recursive case 
    if(cm->sub_component) {
        list = compile_expression_component(cm->sub_component);

    // case case, reach LHS side of expression Component 
    } else {
        list = init_ByteCodeList();
    }

    assert(list);
    ByteCode *instruction = NULL;

    // TODO
    switch(cm->type) {

        // terminal constants
        case NUMERIC_CONSTANT: {
            
            instruction = init_ByteCode(LOAD_CONST);
            RtObject *number_constant = init_RtObject(NUMBER_TYPE, NULL);
            number_constant->data.Number.number=cm->meta_data.numeric_const;
            instruction->data.LOAD_CONST.constant = number_constant;
            break;
        }

        case STRING_CONSTANT: {
            instruction = init_ByteCode(LOAD_CONST);

            RtObject *string_constant = init_RtObject(STRING_TYPE, NULL);
            string_constant->data.String.string = malloc_string_cpy(NULL, cm->meta_data.string_literal);

            instruction->data.LOAD_CONST.constant = string_constant;

            break;
        }

        case LIST_CONSTANT: {
            
            const int list_length = cm->meta_data.list_const.list_length;
            ExpressionNode **elements = cm->meta_data.list_const.list_elements;

            ByteCodeList *compiled_seq = compile_exps_sequence(elements, list_length);
            list = concat_bytecode_lists(list, compiled_seq);

            instruction = init_ByteCode(CREATE_LIST);
            instruction->data.CREATE_LIST.list_length=list_length;

            break;
        }

        case NULL_CONSTANT: {
            
            instruction = init_ByteCode(LOAD_CONST);
            instruction->data.LOAD_CONST.constant = init_RtObject(NULL_TYPE, NULL);

            break;
        }

        case HASHMAP_CONSTANT: {
            
            const int key_val_count = cm->meta_data.HashMap.size;
            KeyValue **pairs = cm->meta_data.HashMap.pairs;

            // Loads key value pairs onto the bytecode 
            for(int i=0; i < key_val_count; i++) {
                ByteCodeList *key = compile_expression(pairs[i]->key);
                ByteCodeList *val = compile_expression(pairs[i]->value);

                list = concat_bytecode_lists(list, concat_bytecode_lists(key,val));
            }

            instruction = init_ByteCode(CREATE_MAP);
            instruction->data.CREATE_MAP.map_size=key_val_count;

            break;
        }

        case HASHSET_CONSTANT: {
            const int set_size = cm->meta_data.list_const.list_length;
            ExpressionNode **elements = cm->meta_data.list_const.list_elements;

            ByteCodeList *compiled_seq = compile_exps_sequence(elements, set_size);
            list = concat_bytecode_lists(list, compiled_seq);

            instruction = init_ByteCode(CREATE_SET);
            instruction->data.CREATE_SET.set_size = set_size;

            break;
        }

        // special cases

        case VARIABLE: {
            // if its non terminal variable 
            if(cm->sub_component) {
                const char* varname = cm->meta_data.variable_reference;
                instruction = init_ByteCode(LOAD_ATTRIBUTE);
                instruction->data.LOAD_ATTR.attribute_name=malloc_string_cpy(NULL, varname);
                instruction->data.LOAD_ATTR.str_length = strlen(varname);

            // if its a variable
            } else {
                instruction = init_ByteCode(LOAD_VAR);
                instruction->data.LOAD_VAR.variable = malloc_string_cpy(NULL, cm->meta_data.variable_reference);
            }
            break;
        }

        case LIST_INDEX: {
            ExpressionNode *exp = cm->meta_data.list_index;
            
            ByteCodeList *compiled_exp = compile_expression(exp);

            list = concat_bytecode_lists(list, compiled_exp);
            
            instruction = init_ByteCode(LOAD_INDEX);

            break;
        }

        // TODO
        case FUNC_CALL: {
            int arg_count = cm->meta_data.func_data.args_num;
            ExpressionNode **args = cm->meta_data.func_data.func_args;
            ByteCodeList *compiled_args = compile_exps_sequence(args, arg_count);

            list = concat_bytecode_lists(list, compiled_args);

            instruction = init_ByteCode(FUNCTION_CALL);
            instruction->data.FUNCTION_CALL.arg_count = arg_count;

            break;
        }

        // TODO 
        case INLINE_FUNC: {

            
            break;
        }

        default:
            break;

    }
    assert(instruction);
    _add_bytecode(list, instruction);

    return list;
}

/* Recursively compiles expression */
ByteCodeList *compile_expression(ExpressionNode* root) {
    if(!root) {
        return NULL;
    }
    // Base case (Reached a leaf)
    if(root->type == VALUE) {
        // TODO
        return compile_expression_component(root->component);
    }

    // Recursive cases

    ByteCodeList *list = 
    // order of arguments important 
    concat_bytecode_lists(
        compile_expression(root->LHS),
        compile_expression(root->RHS)
    );

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
    case LOGICAL_AND:
        operation = init_ByteCode(LOGICAL_AND_VARS_OP);
        break;
    case LOGICAL_OR:
        operation = init_ByteCode(LOGICAL_OR_VARS_OP);
        break;

    case LOGICAL_NOT_VARS_OP:
        operation = init_ByteCode(LOGICAL_NOT_VARS_OP);
        break;
    default:
        break;
    }

    assert(list);
    _add_bytecode(list, operation);

    // Special Case if entire expression is negated 
    if(root->negation) {
        _add_bytecode(list, init_ByteCode(LOGICAL_NOT_VARS_OP));
    }

    return list;
}

/* Compiles a Code Block TODO */
ByteCodeList* compiled_code_body(AST_List *body) {

    ByteCodeList *list = NULL;

    AST_node *node = body->head;
    while(node) {
        switch (node->type)
        {
        case VAR_DECLARATION: {
            ByteCodeList *compiled_rhs = compile_expression(node->ast_data.exp);

            list = concat_bytecode_lists(list, compiled_rhs);
            
            char *varname = node->identifier.declared_var;

            ByteCode *instruction = init_ByteCode(CREATE_VAR);
            instruction->data.CREATE_VAR.new_var_name= malloc_string_cpy(NULL, varname);
            instruction->data.CREATE_VAR.str_length= strlen(varname);

            _add_bytecode(list, instruction);
            break;
        }

        case VAR_ASSIGNMENT: {
            ByteCodeList *compiled_rhs = compile_expression(node->ast_data.exp);
            ByteCodeList *compiled_lhs = compile_expression_component(node->identifier.expression_component);
            
            list = concat_bytecode_lists(list, concat_bytecode_lists(compiled_lhs, compiled_rhs));

            ByteCode* instruction = init_ByteCode(MUTATE_VAR);

            _add_bytecode(list, instruction);
            break;
        }


        // TODO more cases ..
        
        default:
            break;
        }


        node = node->next;
    }
    

    return list;


}

/* Frees ByteCodeList */
void free_ByteCodeList(ByteCodeList *list) {
    //  TODO

}

/* Frees ByteCode struct */
void free_ByteCode(ByteCode *bytecode) {
    // TODO

}


/* Prints out Runtime Object */
void deconstruct_RtObject(RtObject *obj) {
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
    case LIST_TYPE:
    case OBJECT_TYPE:
    case FUNCTION_TYPE:
    case HASHMAP_TYPE:
    case HASHSET_TYPE:
        printf(" Not Implemented \n");

    default:
        break;
    }
}

/* Deconstruct bytecode by printing it out */
void deconstruct_bytecode(ByteCodeList* bytecode) {

    for(int i=0 ; i < bytecode->pg_length; i++) {
        ByteCode *instrc = bytecode->code[i];

        printf("%d      ", i);

        switch (instrc->code)
        {
        case LOAD_CONST:
            printf("LOAD_CONST");
            deconstruct_RtObject(instrc->data.LOAD_CONST.constant);
            break;
        case LOAD_VAR:
            printf("LOAD_VAR: '%s'\n", instrc->data.LOAD_VAR.variable);
            break;
        case MUTATE_VAR:
            printf("MUTATE_VAR\n");
            break;
        case CREATE_VAR:
            printf("CREATE_VAR: '%s' \n", instrc->data.CREATE_VAR.new_var_name);
            break;
        case CREATE_LIST:
            printf("CREATE_LIST: Length %d \n", instrc->data.CREATE_LIST.list_length);
            break;
        case CREATE_SET:
            printf("CREATE_SET: Size %d \n", instrc->data.CREATE_SET.set_size);
            break;
        case CREATE_MAP:
            printf("CREATE_MAP: Size %d \n", instrc->data.CREATE_MAP.map_size);
            break;
        case LOAD_ATTRIBUTE:
            printf("LOAD_ATTRIBUTE: Attribute: '%s'\n", instrc->data.LOAD_ATTR.attribute_name);
            break;
        case LOAD_INDEX:
            printf("LIST_INDEX\n");
            break;
        case FUNCTION_CALL:
            printf("FUNCTION_CALL: %d Arguments \n", instrc->data.FUNCTION_CALL.arg_count);
            break;
        case CLOSURE: {
            printf("CLOSURE: { ");
            for(int i=0; i < instrc->data.CLOSURE.closure_count; i++) {
                printf(" %s,", instrc->data.CLOSURE.closure_vars[i]);
            }

            printf(" }\n");
            break;
        }
        case CONST_JUMP: 
            printf("CONST_JUMP\n");
            break;
        case OFFSET_JUMP:
            printf("OFFSET_JUMP\n");
            break;
        case CONDITIONAL_OFFSET_JUMP:
            printf("CONDITONAL_OFFSET_JUMP\n");
            break;
        case ADD_VARS_OP:      
            printf("ADD_VARS_OP\n");
            break;  
        case SUB_VARS_OP:  
            printf("SUB_VARS_OP\n");
            break;      
        case MULT_VARS_OP: 
            printf("MULT_VARS_OP\n");
            break; 
        case DIV_VARS_OP:  
            printf("DIV_VARS_OP\n");
            break; 
        case MOD_VARS_OP: 
            printf("MOD_VARS_OP\n");
            break;       
        case BITWISE_VARS_AND_OP:
            printf("BITWISE_VARS_AND_OP\n");
            break;
        case BITWISE_VARS_OR_OP: 
            printf("BITWISE_VARS_OR_OP\n");
            break;
        case BITWISE_XOR_VARS_OP:
            printf("BITWISE_XOR_VARS_OP\n");
            break;
        case SHIFT_LEFT_VARS_OP: 
            printf("SHIFT_LEFT_VARS_OP\n");
            break;
        case SHIFT_RIGHT_VARS_OP:
            printf("SHIFT_RIGHT_VARS_OP\n");
            break;
        case GREATER_THAN_VARS_OP:
            printf("GREATER_THAN_VARS_OP\n");
            break;
        case GREATER_EQUAL_VARS_OP:
            printf("GREATER_EQUAL_VARS_OP\n");
            break;
        case LESSER_THAN_VARS_OP:
            printf("LESSER_THAN_VARS_OP\n");
            break;
        case LESSER_EQUAL_VARS_OP:
            printf("LESSER_EQUAL_VARS_OP\n");
            break;
        case EQUAL_TO_VARS_OP:   
            printf("EQUAL_TO_VARS_OP\n");
            break;
        case LOGICAL_AND_VARS_OP:
            printf("LOGICAL_AND_VARS_OP\n");
            break;
        case LOGICAL_OR_VARS_OP: 
            printf("LOGICAL_OR_VARS_OP\n");
            break;
        case LOGICAL_NOT_VARS_OP:
            printf("LOGICAL_NOT_VARS_OP\n");
            break;
        default:
            break;
        }
    }
}
