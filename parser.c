#include <string.h>
#include "lexer.h"
#include "keywords.h"
#include "parser.h"

/* Mallocs if type node for parse tree */
static inline struct if_node *malloc_if_node(
    struct parse_tree_node *_branch_condition,
    struct parse_tree_node *_else_block,
    struct parse_tree_node *_if_body,
    struct parse_tree_node *_else_if_block)
{

    struct if_node *_if_node = (struct if_node *)malloc(sizeof(struct if_node));
    _if_node->_branch_condition = _branch_condition;
    _if_node->_else_block = _else_block;
    _if_node->_if_body;
    _if_node->_else_if_block = _else_if_block;

    return _if_node;
}

/* Mallocs for type node for parse tree, currently will not be implemented */
static inline struct for_node *malloc_for_node() { return NULL; }

/* Mallocs while type node for parse tree */
static inline struct if_node *malloc_while_node(
    struct parse_tree_node *_branch_condition,
    struct parse_tree_node *_while_body)
{

    struct while_node *_while_node = (struct while_node *)malloc(sizeof(struct while_node));
    _while_node->_branch_condition = _branch_condition;
    _while_node->_while_body = _while_body;

    return _while_node;
}

/* Mallocs else type node for parse tree */
static inline struct else_node *malloc_else_node(
    struct parse_tree_node *_branch_condition,
    struct parse_tree_node *_else_body)
{

    struct else_node *_else_node = (struct else_node *)malloc(sizeof(struct else_node));
    _else_node->_branch_condition = _branch_condition;
    _else_node->_else_body = _else_body;

    return _else_node;
}

/* Mallocs function call type node for parse tree */
static inline struct func_call_node *malloc_func_call_node(struct parse_tree_node **_args, char *ident)
{

    struct func_call_node *_func_call_node = (struct func_call_node *)malloc(sizeof(struct func_call_node));
    _func_call_node->args = _args;
    _func_call_node->ident = ident;

    return _func_call_node;
}

/* Mallocs function declaration type node for parse tree */
static inline struct func_declaration_node *malloc_else_node(
    enum variable_types _return_type,
    struct parse_tree_node **_args,
    char *ident)
{

    struct func_declaration_node *_func_declaration_node =
        (struct func_declaration_node *)malloc(sizeof(struct func_declaration_node));
    _func_declaration_node->return_type = _return_type;
    _func_declaration_node->args = _args;
    _func_declaration_node->ident = ident;

    return _func_declaration_node;
}

/* Mallocs function declaration type node for parse tree */
static inline struct func_declaration_node *malloc_func_declaration_node(
    enum variable_types _return_type,
    struct parse_tree_node **_args,
    char *ident)
{

    struct func_declaration_node *_func_declaration_node =
        (struct func_declaration_node *)malloc(sizeof(struct func_declaration_node));
    _func_declaration_node->return_type = _return_type;
    _func_declaration_node->args = _args;
    _func_declaration_node->ident = ident;

    return _func_declaration_node;
}

/* Mallocs variable declaration type node for parse tree */
static inline struct var_declaration_node *malloc_var_assignment_node(
    struct parse_tree_node *_assign_to,
    char *ident)
{

    struct var_declaration_node *_var_declaration_node = (struct var_declaration_node *)malloc(sizeof(struct var_declaration_node));

    _var_declaration_node->ident = ident;
    _var_declaration_node->_assign_to = _assign_to;

    return _var_declaration_node;
}

/* Mallocs variable assignment type node for parse tree */
static inline struct var_assignment_node *malloc_var_assignment_node(
    struct parser_tree_node *_assign_to,
    char *ident)
{

    struct var_assignment_node *_var_assignment_node = (struct var_assignment_node *)malloc(sizeof(struct var_assignment_node));

    _var_assignment_node->ident = ident;
    _var_assignment_node->_assign_to = _assign_to;

    return _var_assignment_node;
}

/* Mallocs variable reference type node for the parser tree */
static inline struct var_reference_node *malloc_var_reference_node(
    enum variable_types type,
    char *ident)
{
    struct var_reference_node *_var_reference_node = (struct var_reference_node *)malloc(sizeof(struct var_reference_node));
    _var_reference_node->type = type;
    _var_reference_node->ident = ident;
    return _var_reference_node;
}

/* Mallocs bool expression type node for parse tree */
static inline struct bool_expression_node *malloc_bool_expression_node(
    struct parse_tree_node *left,
    struct parse_tree_node *right,
    enum bool_expression_types type)
{
    struct bool_expression_node *_bool_expression_node = (struct bool_expression_node *)malloc(sizeof(struct bool_expression_node));
    _bool_expression_node->left = left;
    _bool_expression_node->right = right;
    _bool_expression_node->type = type;
    return _bool_expression_node;
}

/* Mallocs binary i.e math operations (+_-* ...) type node for parse tree */
static inline struct bin_operation_node *malloc_bin_operation_node(
    struct parse_tree_node *left,
    struct parse_tree_node *right,
    enum bin_operation_types type)
{

    struct bin_operation_node *_bin_operation_node = (struct bin_operation_node *)malloc(sizeof(struct bin_operation_node));
    _bin_operation_node->left = left;
    _bin_operation_node->right = right;
    _bin_operation_node->type = type;

    return _bin_operation_node;
}

/* Mallocs a literal (constant) value reference type node for the parse tree */
static inline struct literal_value_node *malloc_literal_value_node(
    char *ident,
    enum literal_value_type type)
{
    struct literal_value_node *_literal_value_node = (struct literal_value_node *)malloc(sizeof(struct literal_value_node));
    _literal_value_node->ident = ident;
    _literal_value_node->type = type;
    return _literal_value_node;
}

/* Mallocs the top level struct for a parse tree node, does NOT set the data union */
static inline struct parse_tree_node *malloc_parse_tree_node(
    struct parse_tree_node *_next,
    struct parse_tree_node *_parent,
    enum parse_tree_type *_type
) {
    struct parse_tree_node *_parser_tree_node = (struct parse_tree_node*)malloc(sizeof(struct parse_tree_node));
    _parser_tree_node->next=_next;
    _parser_tree_node->parent=_parent;
    _parser_tree_node->type=_type;

    return _parser_tree_node;
}

/* This function is a helper that will call the syntax_analysis function with the proper
args depending on the type of keyword */
void call_keyword_spec_parser_args(int starting_index, struct lexeme_array_list *arrlist, enum keyword_type type) {
    if(type == FUNC) {
    

    } else if(type == WHILE) {

    /* */
    } else if(type == IF) {

    /* else if block */
    } else if(type == ELSE && get_keyword_type(arrlist->list[starting_index+1]->ident) == IF) {
        
    } else if(type == ELSE) {

    } else if(type == LET) {
        
    }
}

// Will be Resursive
void syntax_analysis(int index, struct lexeme_array_list *arrlist)
{
    struct lexeme **ptr = arrlist->list[index];

    while (ptr[index] != NULL)
    {
        if (ptr[index]->type == KEYWORD)
        {
            const char *keyword = (*ptr)->ident;
            enum keyword_type type = get_keyword_type(keyword);

            
            
        }

        ++index;
    }
}