#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "keywords.h"
#include "lexer.h"
#include "parser.h"

/// @brief State of the parser (lexeme list and the lexeme pointer)
static int token_ptr = -1;
static struct lexeme_array_list *arrlist = NULL;

/* Resets the token_ptr and arrlist fields */
void reset_parser_state()
{
    token_ptr = -1;
    arrlist = NULL;
}

/* Modifies the parser state */
void set_parser_state(int _token_ptr, struct lexeme_array_list *_arrlist)
{
    token_ptr = _token_ptr;
    arrlist = _arrlist;
}

/* index should point to the first occurence of the numeric constant */
bool is_numeric_const_double(int index)
{
    struct lexeme **list = arrlist->list;
    return list[index]->type == NUMERIC_LITERAL &&
           list[++index]->type == DOT &&
           list[++index]->type == NUMERIC_LITERAL;
}

/* Checks if lexeme_type enum is in list */
bool is_lexeme_in_list(
    enum lexeme_type type, 
    enum lexeme_type list[], 
    const int list_length) {

    for(int i=0; i < list_length; i++) 
        if(list[i] == type) return true;
    
    return false;
}

/* Mallocs malloc_exp_node setting all fields to default values */
struct expression_node *malloc_expression_node()
{
    struct expression_node *node = malloc(sizeof(struct expression_node));
    node->LHS = NULL;
    node->RHS = NULL;
    node->ident = NULL;
    node->attribute_arrow = NULL;
    node->args_num = -1;
    node->type = -1;
    return node;
}

double compute_exp(struct expression_node *root)
{
    switch (root->type)
    {
    /* Base cases */
    case LIST_INDEX:
        return compute_exp(root->meta_data.list_index);
    case VAR:
        return 10.0; // TODO -> TEMP VALUE
    case FUNC_CALL:
        return 20.0; // TODO -> TEMP VALUE
    case INTEGER_CONSTANT:
        return root->meta_data.integer_const;
    case DOUBLE_CONSTANT:
        return root->meta_data.double_const;

    /* Math operators */
    case MULT:
        return compute_exp(root->LHS) * compute_exp(root->RHS);
    case DIV:
        return compute_exp(root->LHS) / compute_exp(root->RHS);
    case PLUS:
        return compute_exp(root->LHS) + compute_exp(root->RHS);
    case MINUS:
        return compute_exp(root->LHS) - compute_exp(root->RHS);
    case MOD:
    {
        double lhs_d = compute_exp(root->LHS);
        double rhs_d = compute_exp(root->RHS);
        return lhs_d - floor(lhs_d / rhs_d) * rhs_d;
    }

    /* boolean operators */
    case LOGICAL_AND:
        return (int)compute_exp(root->LHS) && (int)compute_exp(root->RHS);
    case LOGICAL_OR:
        return (int)compute_exp(root->LHS) || (int)compute_exp(root->RHS);
    case EQUAL_TO:
        return (int)(compute_exp(root->LHS) == compute_exp(root->RHS));
    case GREATER_THAN:
        return (int)(compute_exp(root->LHS) > compute_exp(root->RHS));
    case LESSER_THAN:
        return (int)(compute_exp(root->LHS) < compute_exp(root->RHS));

    /* Bitwise operators */
    case BITWISE_AND:
        return (int)compute_exp(root->LHS) & (int)compute_exp(root->RHS);
    case BITWISE_OR:
        return (int)compute_exp(root->LHS) | (int)compute_exp(root->RHS);
    case SHIFT_LEFT:
        return (int)compute_exp(root->LHS) << (int)compute_exp(root->RHS);
    case SHIFT_RIGHT:
        return (int)compute_exp(root->LHS) >> (int)compute_exp(root->RHS);
    case BITWISE_XOR:
        return (int)compute_exp(root->LHS) ^ (int)compute_exp(root->RHS);

    default:
        return 0.0;
    }
}

/* Recursively frees binary expression parse tree */
void free_expression_tree(struct expression_node *root)
{
    if (!root)
        return;

    free_expression_tree(root->attribute_arrow);
    switch (root->type)
    {
    case INTEGER_CONSTANT:
    case DOUBLE_CONSTANT:
        free(root);
        return;
    case LIST_INDEX:
        free_expression_tree(root->meta_data.list_index);
        free(root->ident);
        free(root);
        return;
    case VAR:
        free(root->ident);
        free(root);
        return;
    case FUNC_CALL: {
        for(int i=0; i < root->args_num; i++)
            free_expression_tree(root->meta_data.func_args[i]);
        
        free(root->meta_data.func_args);
        free(root->ident);
        free(root);
        return;
    }
    default:
        free_expression_tree(root->LHS);
        free_expression_tree(root->RHS);
        free(root);
        return;
    }
}

#define DEFAULT_ARG_LIST_LENGTH 4
// will parse function arguments
// token_ptr should point on the first token inside the parenthesis
// returned array ends with NULL pointer
struct expression_node **parse_function_args() {
    struct expression_node **args = malloc(sizeof(struct expression_node*)*DEFAULT_ARG_LIST_LENGTH);

    int arg_list_max_length=DEFAULT_ARG_LIST_LENGTH;
    int arg_count=0;

    enum lexeme_type seperators[]={COMMA, CLOSING_PARENTHESIS};

    while(arrlist->list[token_ptr-1]->type != CLOSING_PARENTHESIS){
        struct expression_node *tmp=parse_expression(NULL,NULL,seperators,2);

        //if no arguments are provided
        if(!tmp) break;

        args[arg_count++]=tmp;
        
        if(arg_list_max_length == arg_count) {
            struct expression_node **new_arg_list = malloc(sizeof(struct expression_node*) * 2);
            arg_list_max_length*=2;

            for(int i=0; i < arg_count; i++) 
                new_arg_list[i]=args[i];
            
            free(args);
            args=new_arg_list;
        }
    }

    args[arg_count]=NULL;
    return args;
}

/* Gets the length of the argument list (ends with a NULL pointer)*/
int get_argument_count(struct expression_node **args) {
    int length=0;
    for(int i=0; args[i] != NULL; i++) 
        length++;
    return length;
}

// will parse identifer
// assumes arrlist->list[token_ptr]->type == IDENTIFER
struct expression_node *parse_identifer()
{
    struct lexeme **list = arrlist->list;

    // mallocs node
    struct expression_node *node = malloc_expression_node();

    // creates copy of identifier string
    char* ident_cpy = malloc(sizeof(char)*strlen(list[token_ptr]->ident));
    node->ident=strcpy(ident_cpy, list[token_ptr]->ident);

    // function identifier
    if (list[token_ptr + 1]->type == OPEN_PARENTHESIS) {
        token_ptr +=2;
        node->type = FUNC_CALL;
        node->meta_data.func_args=parse_function_args();
        node->args_num=get_argument_count(node->meta_data.func_args);
    } 
    // list identifier
    else if (list[token_ptr + 1]->type == OPEN_CURLY_BRACKETS)
    {
        token_ptr += 2;
        node->type = LIST_INDEX;
        enum lexeme_type end_of_exp[] = {CLOSING_SQUARE_BRACKETS};
        node->meta_data.list_index = parse_expression(NULL, NULL, end_of_exp, 1);
    }
    // standalone variable
    else
    {
        node->type = VAR;
        token_ptr += 1;
    }

    // if identifer as a arrow attribute
    if (list[token_ptr]->type == ATTRIBUTE_ARROW &&
        list[token_ptr + 1]->type == IDENTIFIER)
    {

        token_ptr += 1;
        node->attribute_arrow = parse_identifer();
    }

    return node;
}

// uses reverse polish notation
struct expression_node *parse_expression(
    struct expression_node *LHS,
    struct expression_node *RHS,
    enum lexeme_type ends_of_exp[],
    const int ends_of_exp_length)
{

    assert(token_ptr != -1 && arrlist != NULL && arrlist->len > 1);
    struct lexeme **list = arrlist->list;

    if (list[token_ptr]->type == END_OF_FILE)
        return LHS;

    // Base case (meet end of expression token)
    if (is_lexeme_in_list(list[token_ptr]->type, ends_of_exp, ends_of_exp_length))
    {
        token_ptr++;
        return LHS;
    }

    // Handles and Computes sub expressions
    if (list[token_ptr]->type == OPEN_PARENTHESIS)
    {
        token_ptr++;
        enum lexeme_type end_of_exp[] = {CLOSING_PARENTHESIS};
        struct expression_node *sub_exp = parse_expression(NULL, NULL, end_of_exp, 1);

        if (!LHS)
            return parse_expression(sub_exp, RHS, ends_of_exp, ends_of_exp_length);
        else
            return parse_expression(LHS, sub_exp, ends_of_exp, ends_of_exp_length);
    }

    if (list[token_ptr]->type == IDENTIFIER)
    {
        struct expression_node *identifer = parse_identifer();

        if (!LHS)
            return parse_expression(identifer, RHS, ends_of_exp, ends_of_exp_length);
        else if (!RHS)
            return parse_expression(LHS, identifer, ends_of_exp, ends_of_exp_length);
        else
        {
            free(identifer);
            return NULL;
        }
    }
    struct expression_node *node = malloc_expression_node();

    // handles constants
    if (list[token_ptr]->type == NUMERIC_LITERAL)
    {
        if (is_numeric_const_double(token_ptr))
        {
            node->type = DOUBLE_CONSTANT;
            double lhs = (double)atoi(list[token_ptr]->ident);
            double fraction = (double)atoi(list[token_ptr + 2]->ident);
            double frac_len = (double)strlen(list[token_ptr + 2]->ident);
            node->meta_data.double_const =
                lhs + (double)(fraction / (pow(10, frac_len)));

            token_ptr += 3;
        }
        else
        {
            node->type = INTEGER_CONSTANT;
            node->meta_data.integer_const = atoi(list[token_ptr]->ident);
            token_ptr++;
        }

        if (!LHS)
            return parse_expression(node, RHS, ends_of_exp, ends_of_exp_length);
        else if (!RHS)
            return parse_expression(LHS, node, ends_of_exp, ends_of_exp_length);
        else
        {
            free(node);
            return NULL;
        }
    }

    // Handles ALL math/bitwise/logical operators
    switch (list[token_ptr]->type)
    {
    case MULT_OP: node->type = MULT; break;
    case MINUS_OP: node->type = MINUS; break;
    case PLUS_OP: node->type = PLUS; break;
    case DIV_OP: node->type = DIV; break;
    case MOD_OP: node->type = MOD; break;
    case GREATER_THAN_OP: node->type = GREATER_THAN; break;
    case LESSER_THAN_OP: node->type = LESSER_THAN; break;
    case EQUAL_TO_OP: node->type = EQUAL_TO; break;
    case LOGICAL_AND_OP: node->type = LOGICAL_AND; break;
    case LOGICAL_OR_OP: node->type = LOGICAL_OR; break;
    case LOGICAL_NOT_OP: node->type = LOGICAL_NOT; break;
    case BITWISE_AND_OP: node->type = BITWISE_AND; break;
    case BITWISE_OR_OP: node->type = BITWISE_OR; break;
    case BITWISE_XOR_OP: node->type = BITWISE_XOR; break;
    case SHIFT_LEFT_OP: node->type = SHIFT_LEFT; break;
    case SHIFT_RIGHT_OP: node->type = SHIFT_RIGHT; break;
    default:
        free(node);
        return LHS;
    }

    node->LHS = LHS;
    node->RHS = RHS;

    token_ptr++;

    return parse_expression(node, NULL, ends_of_exp, ends_of_exp_length);
}