#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "keywords.h"
#include "lexer.h"
#include "parser.h"
#include "types.h"

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
bool is_numeric_const_fractional(int index)
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
    const int list_length)
{

    for (int i = 0; i < list_length; i++)
        if (list[i] == type)
            return true;

    return false;
}

/* Mallocs copy of input string*/
char *malloc_string_cpy(const char* str) {
    char* str_cpy = malloc(sizeof(char)*strlen(str));
    strcpy(str_cpy,str);
    return str_cpy;
}

/* Computes fractional number (whole: integer part, frac: fractional part)*/
double compute_fractional_double(struct lexeme *whole, struct lexeme *frac) {
    assert(whole->type == NUMERIC_LITERAL && frac->type == NUMERIC_LITERAL);
    double lhs = (double) atoi(whole->ident);
    double fraction = (double) atoi(whole->ident);

    return lhs + (double)(fraction / (pow(10, strlen(whole->ident))));
}

/* Mallocs expression component struct */
struct expression_component *malloc_expression_component() {
    struct expression_component *component = malloc(sizeof(struct expression_component));
    component->attribute_arrow=NULL;
    component->sub_component=NULL;
    component->type=-1;
    return component;
}

/* Mallocs malloc_exp_node setting all fields to default values */
struct expression_node *malloc_expression_node()
{
    struct expression_node *node = malloc(sizeof(struct expression_node));
    node->LHS = NULL;
    node->RHS = NULL;
    node->component = NULL;
    node->type=-1;
    return node;
}

double compute_exp(struct expression_node *root)
{
    
    /* Base cases (except for LIST_INDEX) */
    if(root->component) {
        switch(root->component->type) {
            case LIST_CONSTANT:
                return 5.0;
            case NUMERIC_CONSTANT:
                return root->component->meta_data.numeric_const;
            case STRING_CONSTANT:
                return strlen(root->component->meta_data.string_literal);
            case VARIABLE:
                return 10.0;
            case FUNC_CALL:
                return 20.0;

            //recursive call 
            case LIST_INDEX:
                return compute_exp(root->component->meta_data.list_index);
            default:
                break;  
        } 
    }
    switch (root->type)
    {
    /* Expression operators, i.e recursive calls  */
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

/* Recursively frees expression component struct */
void free_expression_component(struct expression_component *component) {
    if(!component)
        return;
    
    free_expression_component(component->attribute_arrow);
    free_expression_component(component->sub_component);

    switch(component->type) {
        case NUMERIC_CONSTANT: {
            free(component);
            return;
        } 
        case STRING_CONSTANT: {
            free(component->meta_data.string_literal);
            free(component);
            return;
        } 
        // TODO TO BE IMPLEMENTED
        case LIST_CONSTANT: {
            return;
        }
        case VARIABLE: {
            free(component->meta_data.ident);
            free(component);
            return;
        }
        case FUNC_CALL: {
            for(int i=0; i < component->meta_data.func_data.args_num; i++) 
                free_expression_tree(component->meta_data.func_data.func_args[i]);

            free(component->meta_data.func_data.func_args);
            free(component);  
            break;      
        }
        case LIST_INDEX: {
            free_expression_tree(component->meta_data.list_index);
            free(component);
            break;
        }
    }
}

/* Recursively frees binary expression parse tree */
void free_expression_tree(struct expression_node *root)
{
    if (!root)
        return;

    free_expression_tree(root->LHS);
    free_expression_tree(root->RHS);
    free_expression_component(root->component);
    free(root);
}


#define DEFAULT_ARG_LIST_LENGTH 4
// will parse function arguments
// token_ptr should point on the first token inside the parenthesis
// returned array ends with NULL pointer
struct expression_node **parse_function_args()
{
    struct expression_node **args = malloc(sizeof(struct expression_node *) * DEFAULT_ARG_LIST_LENGTH);

    int arg_list_max_length = DEFAULT_ARG_LIST_LENGTH;
    int arg_count = 0;

    enum lexeme_type seperators[] = {COMMA, CLOSING_PARENTHESIS};

    while (arrlist->list[token_ptr - 1]->type != CLOSING_PARENTHESIS)
    {
        struct expression_node *tmp = parse_expression(NULL, NULL, seperators, 2);

        // if all argument tokens have been consumed
        if (!tmp)
            break;

        args[arg_count++] = tmp;

        if (arg_list_max_length == arg_count)
        {
            arg_list_max_length *= 2;
            struct expression_node **new_arg_list = malloc(sizeof(struct expression_node *) * arg_list_max_length);

            for (int i = 0; i < arg_count; i++)
                new_arg_list[i] = args[i];

            free(args);
            args = new_arg_list;
        }
    }

    args[arg_count] = NULL;
    return args;
}

/* Gets the length of the argument list (ends with a NULL pointer)*/
int get_argument_count(struct expression_node **args)
{
    int length = 0;
    for (int i = 0; args[i] != NULL; i++)
        length++;
    return length;
}


/* 
This function is responsible for recursively parsing expression components (i.e num in -> num 10 *)
NUMERIC_CONSTANT -> number
STRING_CONSTANT -> string
LIST_CONSTANT -> list
FUNC_CALL -> func call -> (arg1, ..., arg_n)
LIST_INDEX -> [...]

*/
struct expression_component *parse_expression_component(
    struct expression_component *parent,
    int rec_lvl
) {
    struct lexeme **list = arrlist->list;
    assert(rec_lvl >= 0);

    //Base cases
    if(list[token_ptr]->type == END_OF_FILE) return parent;
    if(parent) {
        //if parent has attribute arrow, then rest of the expression as been parsed
        if (parent->attribute_arrow) return parent;
        //handles constant values that cannot have list indexing or function calls
        if( parent->type == NUMERIC_CONSTANT || 
            parent->type == STRING_CONSTANT ||
            parent->type == LIST_CONSTANT) {

            return parent;        
        }

        if((parent->type == FUNC_CALL || parent->type == LIST_INDEX || parent->type == VARIABLE) &&
        (list[token_ptr]->type != OPEN_PARENTHESIS && 
        list[token_ptr]->type != OPEN_SQUARE_BRACKETS) &&
        list[token_ptr]->type != ATTRIBUTE_ARROW) {
            return parent;
        }
    }

    // parses attribute expression component if present
    if(list[token_ptr]->type == ATTRIBUTE_ARROW) {
        // assert(list[token_ptr+1]->type == IDENTIFIER);
        token_ptr++;
        // component=parse_expression_component(component, ++rec_lvl);
    }

    struct expression_component *component = malloc_expression_component();

    //CONSTANT LIST
    if(rec_lvl == 0 && !parent && list[token_ptr]->type == OPEN_SQUARE_BRACKETS)  {
        //TODO TO BE IMPLEMENTED
    
    //numbers
    } else if(list[token_ptr]->type == NUMERIC_LITERAL) {
        component->type=NUMERIC_CONSTANT;
        if(is_numeric_const_fractional(token_ptr)) {
            component->meta_data.numeric_const=compute_fractional_double(list[token_ptr], list[token_ptr+2]);
            token_ptr+=2;
        }
        component->meta_data.numeric_const=(double)atoi(list[token_ptr]->ident);

        token_ptr++;
    //strings
    } else if(list[token_ptr]->type == STRING_LITERALS) {
        component->type=STRING_CONSTANT;
        component->meta_data.string_literal=malloc_string_cpy(list[token_ptr]->ident);
        token_ptr++;

    //some variable reference
    } else if(list[token_ptr]->type == IDENTIFIER) {
        component->type=VARIABLE;
        component->meta_data.ident=malloc_string_cpy(list[token_ptr]->ident);
        token_ptr++;

    // list index
    } else if(list[token_ptr]->type == OPEN_SQUARE_BRACKETS) {
        component->type=LIST_INDEX;
        token_ptr++;
        enum lexeme_type end_of_exp[] = {CLOSING_SQUARE_BRACKETS};
        component->meta_data.list_index=parse_expression(NULL,NULL,end_of_exp,1);

    // function call
    } else if(list[token_ptr]->type == OPEN_PARENTHESIS) {
        component->type=FUNC_CALL;
        token_ptr++;
        struct expression_node **args = parse_function_args();
        component->meta_data.func_data.func_args = args;
        component->meta_data.func_data.args_num = get_argument_count(args);
    } 
    

    component->sub_component=parent;


    return parse_expression_component(component, ++rec_lvl);
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

    /*        if (!LHS)
            return parse_expression(sub_exp, RHS, ends_of_exp, ends_of_exp_length);
        else
            return parse_expression(LHS, sub_exp, ends_of_exp, ends_of_exp_length);
            */
           
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

    struct expression_node *node = malloc_expression_node();

    if(
    list[token_ptr]->type == IDENTIFIER ||
    list[token_ptr]->type == STRING_LITERALS ||
    list[token_ptr]->type == NUMERIC_LITERAL ||
    list[token_ptr]->type == OPEN_SQUARE_BRACKETS) {
    
        node->component=parse_expression_component(NULL,0);
        node->type=VALUE;

        if (!LHS)
            return parse_expression(node, RHS, ends_of_exp, ends_of_exp_length);
        else
            return parse_expression(LHS, node, ends_of_exp, ends_of_exp_length);
    }


    // Handles ALL math/bitwise/logical operators
    switch (list[token_ptr]->type)
    {
    case MULT_OP:
        node->type = MULT;
        break;
    case MINUS_OP:
        node->type = MINUS;
        break;
    case PLUS_OP:
        node->type = PLUS;
        break;
    case DIV_OP:
        node->type = DIV;
        break;
    case MOD_OP:
        node->type = MOD;
        break;
    case GREATER_THAN_OP:
        node->type = GREATER_THAN;
        break;
    case LESSER_THAN_OP:
        node->type = LESSER_THAN;
        break;
    case EQUAL_TO_OP:
        node->type = EQUAL_TO;
        break;
    case LOGICAL_AND_OP:
        node->type = LOGICAL_AND;
        break;
    case LOGICAL_OR_OP:
        node->type = LOGICAL_OR;
        break;
    case LOGICAL_NOT_OP:
        node->type = LOGICAL_NOT;
        break;
    case BITWISE_AND_OP:
        node->type = BITWISE_AND;
        break;
    case BITWISE_OR_OP:
        node->type = BITWISE_OR;
        break;
    case BITWISE_XOR_OP:
        node->type = BITWISE_XOR;
        break;
    case SHIFT_LEFT_OP:
        node->type = SHIFT_LEFT;
        break;
    case SHIFT_RIGHT_OP:
        node->type = SHIFT_RIGHT;
        break;
    default:
        free_expression_tree(node);
        return LHS;
    }

    node->LHS = LHS;
    node->RHS = RHS;

    token_ptr++;

    return parse_expression(node, NULL, ends_of_exp, ends_of_exp_length);
}