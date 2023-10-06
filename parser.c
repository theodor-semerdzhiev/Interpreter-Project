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

/* Checks if list1 and list2 have common enums */
bool lexeme_lists_intersect(
    enum lexeme_type list1[],
    const int list1_length,
    enum lexeme_type list2[],
    const int list2_length)
{
    for (int i = 0; i < list1_length; i++)
    {
        for (int j = 0; j < list2_length; j++)
        {
            if (list1[i] == list2[j])
                return true;
        }
    }

    return false;
}

/* Mallocs copy of input string*/
char *malloc_string_cpy(const char *str)
{
    char *str_cpy = malloc(sizeof(char) * strlen(str));
    strcpy(str_cpy, str);
    return str_cpy;
}

/* Computes fractional number (whole: integer part, frac: fractional part)*/
double compute_fractional_double(struct lexeme *whole, struct lexeme *frac)
{
    assert(whole->type == NUMERIC_LITERAL && frac->type == NUMERIC_LITERAL);
    double lhs = (double)atoi(whole->ident);
    double fraction = (double)atoi(frac->ident);
    return lhs + (double)(fraction / (pow(10, strlen(whole->ident))));
}

/* Mallocs expression component struct */
struct expression_component *malloc_expression_component()
{
    struct expression_component *component = malloc(sizeof(struct expression_component));
    component->sub_component = NULL;
    component->type = -1;
    return component;
}

/* Mallocs malloc_exp_node setting all fields to default values */
struct expression_node *malloc_expression_node()
{
    struct expression_node *node = malloc(sizeof(struct expression_node));
    node->LHS = NULL;
    node->RHS = NULL;
    node->component = NULL;
    node->type = -1;
    return node;
}

/* Checks wether lexeme_type is a valid beginning of expression component, 
Is also valid for func keyword (inline functions syntax)
*/
bool is_lexeme_preliminary_expression_token(struct lexeme *lexeme)
{
    return lexeme->type == IDENTIFIER ||
           lexeme->type == OPEN_SQUARE_BRACKETS ||
           lexeme->type == NUMERIC_LITERAL ||
           lexeme->type == STRING_LITERALS ||
           (lexeme->type == KEYWORD && 
           get_keyword_type(lexeme->ident) == FUNC);
}

double compute_exp(struct expression_node *root)
{

    /* Base cases (except for LIST_INDEX) */
    if (root->component)
    {
        switch (root->component->type)
        {
        case LIST_CONSTANT:
        {
            double result = 0.0;
            for (int i = 0; i < root->component->meta_data.list_const.list_length; i++)
            {
                result += compute_exp(root->component->meta_data.list_const.list_elements[i]);
            }
            return result;
        }
        case NUMERIC_CONSTANT:
            return root->component->meta_data.numeric_const;
        case STRING_CONSTANT:
            return strlen(root->component->meta_data.string_literal);
        case VARIABLE:
            return 10.0;
        case FUNC_CALL:
            return 20.0;

        // recursive call
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
    {
        double lhs = compute_exp(root->LHS);
        double rhs = compute_exp(root->RHS);

        return (int)(lhs == rhs);
    }
    case GREATER_THAN:
        return (int)(compute_exp(root->LHS) > compute_exp(root->RHS));
    case GREATER_EQUAL:
        return (int)(compute_exp(root->LHS) >= compute_exp(root->RHS));
    case LESSER_THAN:
        return (int)(compute_exp(root->LHS) < compute_exp(root->RHS));
    case LESSER_EQUAL:
        return (int)(compute_exp(root->LHS) <= compute_exp(root->RHS));

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
void free_expression_component(struct expression_component *component)
{
    if (!component)
        return;

    free_expression_component(component->sub_component);

    switch (component->type)
    {
        case NUMERIC_CONSTANT:
        {
            free(component);
            return;
        }
        case STRING_CONSTANT:
        {
            free(component->meta_data.string_literal);
            free(component);
            return;
        }
        case LIST_CONSTANT:
        {
            for (int i = 0; i < component->meta_data.list_const.list_length; i++)
                free_expression_tree(component->meta_data.list_const.list_elements[i]);

            free(component->meta_data.list_const.list_elements);
            free(component);
            return;
        }
        case VARIABLE:
        {
            free(component->meta_data.ident);
            free(component);
            return;
        }
        case FUNC_CALL:
        {
            for (int i = 0; i < component->meta_data.func_data.args_num; i++)
                free_expression_tree(component->meta_data.func_data.func_args[i]);

            free(component->meta_data.func_data.func_args);
            free(component);
            return;
        }
        case LIST_INDEX:
        {
            free_expression_tree(component->meta_data.list_index);
            free(component);
            return;
        }

        case INLINE_FUNC: {
            free_ast_node(component->meta_data.inline_func);
            
            free(component);
            return;
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

#define DEFAULT_ARG_LIST_LENGTH 6

// will parse individual expressions that are seperated by the seperator lexeme
// end_of_exp is used to mark the end of the expressions sequence
struct expression_node **parse_expressions_by_seperator(
    enum lexeme_type seperator,
    enum lexeme_type end_of_exp)
{
    struct expression_node **args = malloc(sizeof(struct expression_node *) * DEFAULT_ARG_LIST_LENGTH);

    int arg_list_max_length = DEFAULT_ARG_LIST_LENGTH;
    int arg_count = 0;

    enum lexeme_type seperators[] = {seperator, end_of_exp};

    // iterates until end_of_exp lexeme is reached
    while (arrlist->list[token_ptr - 1]->type != end_of_exp)
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
int get_expression_list_length(struct expression_node **args)
{
    int length = 0;
    for (int i = 0; args[i] != NULL; i++)
        length++;
    return length;
}

/*
This function is responsible for recursively parsing expression components (i.e num in -> num 10 *)
rec_lvl (recursion level) should be called with inital value of 0
NUMERIC_CONSTANT -> number
STRING_CONSTANT -> string
LIST_CONSTANT -> list
FUNC_CALL -> func call -> (arg1, ..., arg_n)
LIST_INDEX -> [...]
*/
struct expression_component *parse_expression_component(
    struct expression_component *parent,
    int rec_lvl)
{
    struct lexeme **list = arrlist->list;
    assert(rec_lvl >= 0);

    // Base cases
    if (list[token_ptr]->type == END_OF_FILE)
        return parent;
    if (parent)
    {

        if ((
                parent->type == FUNC_CALL ||
                parent->type == LIST_INDEX ||
                parent->type == NUMERIC_CONSTANT ||
                parent->type == STRING_CONSTANT ||
                parent->type == LIST_CONSTANT ||
                parent->type == VARIABLE ||
                parent->type == INLINE_FUNC) &&
            list[token_ptr]->type != OPEN_PARENTHESIS &&
            list[token_ptr]->type != OPEN_SQUARE_BRACKETS &&
            list[token_ptr]->type != ATTRIBUTE_ARROW)
        {
            return parent;
        }
    }

    // skips token if needed
    if (list[token_ptr]->type == ATTRIBUTE_ARROW)
        token_ptr++;

    struct expression_component *component = malloc_expression_component();


    // Parse list constants
    if (rec_lvl == 0 && !parent && list[token_ptr]->type == OPEN_SQUARE_BRACKETS)
    {
        component->type = LIST_CONSTANT;
        token_ptr++;
        struct expression_node **elements = parse_expressions_by_seperator(COMMA, CLOSING_SQUARE_BRACKETS);
        component->meta_data.list_const.list_elements = elements;
        component->meta_data.list_const.list_length = get_expression_list_length(elements);
    }

    // Parses inline declared functions 
    else if(get_keyword_type(list[token_ptr]->ident) == FUNC) {
        component->type = INLINE_FUNC;
        component->meta_data.inline_func=parse_inline_func(rec_lvl);

    }   
    else if (list[token_ptr]->type == NUMERIC_LITERAL)
    {
        component->type = NUMERIC_CONSTANT;
        if (is_numeric_const_fractional(token_ptr))
        {
            component->meta_data.numeric_const = compute_fractional_double(list[token_ptr], list[token_ptr + 2]);
            token_ptr += 2;
        }
        else
        {
            component->meta_data.numeric_const = (double)atoi(list[token_ptr]->ident);
        }
        token_ptr++;
        // strings
    }
    else if (list[token_ptr]->type == STRING_LITERALS)
    {
        component->type = STRING_CONSTANT;
        component->meta_data.string_literal = malloc_string_cpy(list[token_ptr]->ident);
        token_ptr++;

        // some variable reference
    }
    else if (list[token_ptr]->type == IDENTIFIER)
    {
        component->type = VARIABLE;
        component->meta_data.ident = malloc_string_cpy(list[token_ptr]->ident);
        token_ptr++;

        // list index
    }
    else if (list[token_ptr]->type == OPEN_SQUARE_BRACKETS)
    {
        component->type = LIST_INDEX;
        token_ptr++;
        enum lexeme_type end_of_exp[] = {CLOSING_SQUARE_BRACKETS};
        component->meta_data.list_index = parse_expression(NULL, NULL, end_of_exp, 1);

        // function call
    }
    else if (list[token_ptr]->type == OPEN_PARENTHESIS)
    {
        component->type = FUNC_CALL;
        token_ptr++;
        struct expression_node **args = parse_expressions_by_seperator(COMMA, CLOSING_PARENTHESIS);
        component->meta_data.func_data.func_args = args;
        component->meta_data.func_data.args_num = get_expression_list_length(args);
    } else {
        // TODO -> Invalid token error, handle this 
        free(component);
        return NULL;
    }

    component->sub_component = parent;

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

    if (is_lexeme_preliminary_expression_token(list[token_ptr]))
    {

        node->component = parse_expression_component(NULL, 0);
        node->type = VALUE;

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
    case GREATER_EQUAL_OP:
        node->type = GREATER_EQUAL;
        break;
    case LESSER_THAN_OP:
        node->type = LESSER_THAN;
        break;
    case LESSER_EQUAL_OP:
        node->type = LESSER_EQUAL;
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

/* Mallocs abstract syntac tree node */
struct ast_node *malloc_ast_node()
{
    struct ast_node *node = malloc(sizeof(struct ast_node));
    node->body = NULL;
    node->prev = NULL;
    node->next = NULL;
    node->type = -1;
    return node;
}

/* The function ONLY frees the ast_node itself, not the other nodes that its pointing to */
void free_ast_node(struct ast_node *node)
{
    switch (node->type)
    {
    case VAR_DECLARATION:
    {
        free(node->identifier.declared_var);
        free_expression_tree(node->ast_data.exp);
        free(node);
        return;
    }

    case VAR_ASSIGNMENT:
    {
        free_expression_tree(node->ast_data.exp);
        free_expression_component(node->identifier.expression_component);
        free(node);
        return;
    }

    case IF_CONDITIONAL:
    {
        free_expression_tree(node->ast_data.exp);
        free_ast_list(node->body);
        free(node);
        return;
    }

    case ELSE_IF_CONDITIONAL:
    {
        free_expression_tree(node->ast_data.exp);
        free_ast_list(node->body);
        free(node);
        return;
    }

    case ELSE_CONDITIONAL:
    {
        free_ast_list(node->body);
        free(node);
        return;
    }

    case WHILE_LOOP:
    {
        free_expression_tree(node->ast_data.exp);
        free_ast_list(node->body);
        free(node);
        return;
    }

    case INLINE_FUNCTION_DECLARATION: {
        for (int i = 0; i < node->ast_data.func_args.args_num; i++)
            free_expression_tree(node->ast_data.func_args.func_prototype_args[i]);

        free(node->ast_data.func_args.func_prototype_args);

        // function name does not need to be freed
        free_ast_list(node->body);
        free(node);
        return;
    }
    case FUNCTION_DECLARATION:
    {
        for (int i = 0; i < node->ast_data.func_args.args_num; i++)
            free_expression_tree(node->ast_data.func_args.func_prototype_args[i]);

        free(node->ast_data.func_args.func_prototype_args);

        free_ast_list(node->body);
        free(node->identifier.func_name);
        free(node);
        return;
    }

    case RETURN_VAL:
    {
        free_expression_tree(node->ast_data.exp);
        free(node);
        return;
    }

    case EXPRESSION_COMPONENT:
    {
        free_expression_component(node->identifier.expression_component);
        free(node);
        return;
    }

    case LOOP_TERMINATOR:
    case LOOP_CONTINUATION:
        free(node);
        return;

    default:
        return;
    }
}

/* Mallocs abstract syntax tree list */
struct ast_list *malloc_ast_list()
{
    struct ast_list *list = malloc(sizeof(struct ast_list));
    list->head = NULL;
    list->parent_block = NULL;
    list->tail = NULL;
    list->length = 0;
    return list;
}

/* Recursively frees ast_list struct */
void free_ast_list(struct ast_list *list)
{
    if (!list)
        return;
    struct ast_node *ptr = list->head;

    // frees all ast_nodes in list
    while (ptr)
    {
        struct ast_node *tmp = ptr->next;
        free_ast_node(ptr);
        ptr = tmp;
    }

    free(list);
}

/*
Adds a ast_node to the high abstraction ast_list data structure (i.e linked list)
NOTE: list is volatile, and MUST stay that way to prevent aggressive compiler optimizations from breaking crucial logic
*/

void push_to_ast_list(volatile struct ast_list *list, struct ast_node *node)
{
    if (!list)
        return;

    if (list->head == NULL)
    {
        list->head = node;
        list->tail = node;
        list->head->prev = NULL;
    }
    else
    {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->length++;
}

/* Creates AST node and parses variable declaration */
struct ast_node *parse_variable_declaration(int rec_lvl)
{
    struct lexeme **list = arrlist->list;

    // Sanity checks
    assert(get_keyword_type(list[token_ptr]->ident) == LET);
    assert(list[token_ptr + 1]->type == IDENTIFIER && list[token_ptr + 2]->type == ASSIGNMENT_OP);

    struct ast_node *node = malloc_ast_node();

    node->type = VAR_DECLARATION;
    // gets the var name
    node->identifier.declared_var = malloc_string_cpy(list[token_ptr + 1]->ident);

    // parses its assignment value (i.e an expression)
    token_ptr += 3;
    enum lexeme_type end_of_exp[] = {SEMI_COLON};
    node->ast_data.exp = parse_expression(NULL, NULL, end_of_exp, 1);

    return node;
}

/* Creates AST node and parses while loop block */
struct ast_node *parse_while_loop(int rec_lvl)
{
    struct lexeme **list = arrlist->list;

    // sanity checks
    assert(get_keyword_type(list[token_ptr]->ident) == WHILE);
    assert(list[token_ptr + 1]->type == OPEN_PARENTHESIS);

    struct ast_node *node = malloc_ast_node();

    node->type = WHILE_LOOP;
    token_ptr += 2;

    // parse conditional expression
    enum lexeme_type end_of_exp[] = {CLOSING_PARENTHESIS};
    node->ast_data.exp = parse_expression(NULL, NULL, end_of_exp, 1);

    // recursively parses inner code block
    assert(list[token_ptr]->type == OPEN_CURLY_BRACKETS);
    token_ptr++;
    enum lexeme_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    node->body = parse_code_block(node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/* Create AST node and parses if conditional block */
struct ast_node *parse_if_conditional(int rec_lvl)
{
    struct lexeme **list = arrlist->list;

    // sanity checks
    assert(get_keyword_type(list[token_ptr]->ident) == IF);
    assert(list[token_ptr + 1]->type == OPEN_PARENTHESIS);

    struct ast_node *node = malloc_ast_node();

    node->type = IF_CONDITIONAL;
    token_ptr += 2;

    // parses conditional expression
    enum lexeme_type end_of_exp[] = {CLOSING_PARENTHESIS};
    node->ast_data.exp = parse_expression(NULL, NULL, end_of_exp, 1);

    assert(list[token_ptr]->type == OPEN_CURLY_BRACKETS);

    token_ptr++;

    // recursively parses inner code block
    enum lexeme_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    node->body = parse_code_block(node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/* Parse loop termination (i.e break) */
struct ast_node *parse_loop_termination(int rec_lvl)
{
    struct lexeme **list = arrlist->list;

    assert(get_keyword_type(list[token_ptr]->ident) == BREAK);
    assert(list[token_ptr + 1]->type == SEMI_COLON);
    
    struct ast_node *node = malloc_ast_node();

    node->type = LOOP_TERMINATOR;
    token_ptr+=2;

    return node;
}

/* Parses loop continuation (i.e continue )*/
struct ast_node *parse_loop_continuation(int rec_lvl)
{
    struct lexeme **list = arrlist->list;

    assert(get_keyword_type(list[token_ptr]->ident) == CONTINUE);
    assert(list[token_ptr + 1]->type == SEMI_COLON);

    struct ast_node *node = malloc_ast_node();

    node->type = LOOP_CONTINUATION;
    token_ptr+=2;

    return node;
}

/* Parses return expression (i.e return keyword )*/
struct ast_node *parse_return_expression(int rec_lvl)
{
    struct lexeme **list = arrlist->list;

    assert(get_keyword_type(list[token_ptr]->ident) == RETURN);
                                                                                                        
    struct ast_node *node = malloc_ast_node();

    node->type = RETURN_VAL;
    token_ptr++;
    enum lexeme_type end_of_exp[] = {SEMI_COLON};

    node->ast_data.exp = parse_expression(NULL, NULL, end_of_exp, 1);

    return node;
}

/* Creates AST node and parse else (or else if) conditional block */
struct ast_node *parse_else_conditional(int rec_lvl)
{
    struct lexeme **list = arrlist->list;

    // sanity checks
    assert(get_keyword_type(list[token_ptr]->ident) == ELSE);
    assert(get_keyword_type(list[token_ptr + 1]->ident) == IF || list[token_ptr + 1]->type == OPEN_CURLY_BRACKETS);

    // else if block
    if (get_keyword_type(list[token_ptr + 1]->ident) == IF)
    {
        struct ast_node *node = malloc_ast_node();

        node->type = ELSE_IF_CONDITIONAL;

        // parse conditional expression
        assert(list[token_ptr + 2]->type == OPEN_PARENTHESIS);
        enum lexeme_type end_of_exp[] = {CLOSING_PARENTHESIS};
        token_ptr += 3;
        node->ast_data.exp = parse_expression(NULL, NULL, end_of_exp, 1);

        // recursively parses inner code block
        assert(list[token_ptr]->type == OPEN_CURLY_BRACKETS);
        token_ptr++;
        enum lexeme_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
        node->body = parse_code_block(node, rec_lvl + 1, end_of_block, 1);

        return node;
        // regular else block
    }
    else if (list[token_ptr + 1]->type == OPEN_CURLY_BRACKETS)
    {
        struct ast_node *node = malloc_ast_node();

        node->type = ELSE_CONDITIONAL;

        enum lexeme_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
        token_ptr += 2;
        node->body = parse_code_block(node, rec_lvl + 1, end_of_block, 1);

        return node;
    }
    else
    {
        // bad syntax should not happen
        printf("Wrong syntac for else conditional at line: %d\n", list[token_ptr]->line_num);
        return NULL;
    }

    return NULL;
}

/* Creates AST node and parses function declaration block */
struct ast_node *parse_func_declaration(int rec_lvl)
{
    struct lexeme **list = arrlist->list;

    // sanity checks
    assert(get_keyword_type(list[token_ptr]->ident) == FUNC);
    assert(list[token_ptr + 1]->type == IDENTIFIER);
    assert(list[token_ptr + 2]->type == OPEN_PARENTHESIS);

    struct ast_node *node = malloc_ast_node();

    node->type = FUNCTION_DECLARATION;
    node->identifier.func_name = malloc_string_cpy(list[token_ptr + 1]->ident);
    token_ptr += 3;

    // parses function args
    struct expression_node **args = parse_expressions_by_seperator(COMMA, CLOSING_PARENTHESIS);
    node->ast_data.func_args.func_prototype_args = args;
    node->ast_data.func_args.args_num = get_expression_list_length(args);

    // parses function inner code block
    assert(list[token_ptr]->type == OPEN_CURLY_BRACKETS);
    enum lexeme_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    token_ptr++;
    node->body = parse_code_block(node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/* 
Parses inline function syntax, very similar to regular function declaration parsing.
But not have corresponding identifer */
struct ast_node *parse_inline_func(int rec_lvl) {
    struct lexeme **list = arrlist->list;

    // sanity checks
    assert(get_keyword_type(list[token_ptr]->ident) == FUNC);
    assert(list[token_ptr + 1]->type == OPEN_PARENTHESIS);

    struct ast_node *node = malloc_ast_node();

    node->type = INLINE_FUNCTION_DECLARATION;
    node->identifier.func_name=NULL;

    token_ptr += 2;

    // parses function args
    struct expression_node **args = parse_expressions_by_seperator(COMMA, CLOSING_PARENTHESIS);
    node->ast_data.func_args.func_prototype_args = args;
    node->ast_data.func_args.args_num = get_expression_list_length(args);

    // parses function inner code block
    assert(list[token_ptr]->type == OPEN_CURLY_BRACKETS);
    enum lexeme_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    token_ptr++;
    node->body = parse_code_block(node, rec_lvl + 1, end_of_block, 1);
    
    // Remark: token_ptr points to next token after end_of_block[] token
    return node;
}

/* Recursively parses code block and returns a ast_node (abstract syntax tree node)*/
/* Function will always terminate with the token_ptr pointing on the end exp token + 1*/
struct ast_list *parse_code_block(
    struct ast_node *parent_block,
    int rec_lvl,
    enum lexeme_type ends_of_block[],
    const int ends_of_block_length)
{

    struct lexeme **list = arrlist->list;

    if (is_lexeme_in_list(list[token_ptr]->type, ends_of_block, ends_of_block_length))
    {
        token_ptr++;
        return NULL;
    }

    struct ast_list *ast_list = malloc(sizeof(struct ast_list));

    // loops until it encounters a end of block token
    while (!is_lexeme_in_list(list[token_ptr]->type, ends_of_block, ends_of_block_length))
    {
        switch (get_keyword_type(list[token_ptr]->ident))
        {

            // variable declaration
            case LET:
            {
                struct ast_node *node = parse_variable_declaration(rec_lvl);
                push_to_ast_list(ast_list, node);
                break;
            }
            // WHILE loop
            case WHILE:
            {
                struct ast_node *node = parse_while_loop(rec_lvl);
                push_to_ast_list(ast_list, node);
                break;
            }

            // if conditional
            case IF:
            {
                struct ast_node *node = parse_if_conditional(rec_lvl);
                push_to_ast_list(ast_list, node);
                break;
            }

            // else conditional
            case ELSE:
            {
                struct ast_node *node = parse_else_conditional(rec_lvl);
                push_to_ast_list(ast_list, node);
                break;
            }

            // break (loop termination)
            case BREAK:
            {
                struct ast_node *node = parse_loop_termination(rec_lvl);
                push_to_ast_list(ast_list, node);
                break;
            }

            // continue (loop continuation)
            case CONTINUE:
            {
                struct ast_node *node = parse_loop_continuation(rec_lvl);
                push_to_ast_list(ast_list, node);
                break;
            }

            // return
            case RETURN:
            {
                struct ast_node *node = parse_return_expression(rec_lvl);
                push_to_ast_list(ast_list, node);
                break;
            }

            // function declaration
            case FUNC:
            {
                struct ast_node *node = parse_func_declaration(rec_lvl);
                push_to_ast_list(ast_list, node);
                break;
            }

            default:
            {
                // TODO: Seperate this logic into a seperate function
                // parses variable assignment
                // Grammar: [EXPRESSION COMPONENT] [ASSIGNMENT OP] [EXPRESSION][END OF EXPRESSION]
                // OR parses function calls
                // Grammar [EXPRESSION COMPONENT] -> [IDENTIFER][ARGUMENTS ...][END OF EXPRESSION]
                if (is_lexeme_preliminary_expression_token(list[token_ptr]))
                {
                    struct expression_component *component = parse_expression_component(NULL, 0);

                    assert(list[token_ptr]->type == SEMI_COLON || list[token_ptr]->type == ASSIGNMENT_OP);

                    struct ast_node *node = malloc_ast_node();

                    node->identifier.expression_component = component;
                    

                    if (list[token_ptr]->type == SEMI_COLON)
                    {
                        node->type = EXPRESSION_COMPONENT;
                        token_ptr++;
                    }
                    else if (list[token_ptr]->type == ASSIGNMENT_OP)
                    {
                        node->type = VAR_ASSIGNMENT;
                        token_ptr++;
                        enum lexeme_type end_of_exp[] = {SEMI_COLON};
                        node->ast_data.exp = parse_expression(NULL, NULL, end_of_exp, 1);
                    }

                    push_to_ast_list(ast_list, node);
                }
                else
                {
                    // syntax error TODO
                    printf("BAD VARIABLE ASSIGNMEMENT Line: %d\n", list[token_ptr]->line_num);
                }

                break;
            }
        }
    }

    ast_list->parent_block = parent_block;

    token_ptr++;
    return ast_list;
}
