#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "keywords.h"
#include "parser.h"

/* Mallocs Parser struct */
Parser *malloc_parser()
{
    Parser *parser = malloc(sizeof(Parser));
    parser->error_indicator = false;
    parser->lexeme_list = NULL;
    parser->lines = NULL;
    parser->token_ptr = 0;
    return parser;
}

/* Frees parser */
void free_parser(Parser *parser)
{
    if (!parser)
        return;

    free_line_list(parser->lines);
    free_lexeme_arrlist(parser->lexeme_list);
    free(parser);
}

/* index should point to the first occurence of the numeric constant */
bool is_numeric_const_fractional(Parser *parser, int index)
{
    struct token **list = parser->lexeme_list->list;
    return list[index]->type == NUMERIC_LITERAL &&
           list[++index]->type == DOT &&
           list[++index]->type == NUMERIC_LITERAL;
}

/* Checks if lexeme_type enum is in list */
bool is_lexeme_in_list(
    enum token_type type,
    enum token_type list[],
    const int list_length)
{

    for (int i = 0; i < list_length; i++)
        if (list[i] == type)
            return true;

    return false;
}

/* Checks if list1 and list2 have common enums */
bool lexeme_lists_intersect(
    enum token_type list1[],
    const int list1_length,
    enum token_type list2[],
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
    char *str_cpy = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(str_cpy, str);
    return str_cpy;
}

/* Computes fractional number (whole: integer part, frac: fractional part)*/
double compute_fractional_double(struct token *whole, struct token *frac)
{
    assert(whole->type == NUMERIC_LITERAL && frac->type == NUMERIC_LITERAL);
    double lhs = (double)atoi(whole->ident);
    double fraction = (double)atoi(frac->ident) / (pow(10, strlen(frac->ident)));
    return lhs + fraction;
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
bool is_lexeme_preliminary_expression_token(struct token *lexeme)
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
        free(component->meta_data.variable_reference);
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

    case UNKNOWN_FUNC:
    {
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
    Parser *parser,
    enum token_type seperator,
    enum token_type end_of_exp)
{
    assert(parser);

    struct expression_node **args = malloc(sizeof(struct expression_node *) * DEFAULT_ARG_LIST_LENGTH);
    int arg_list_max_length = DEFAULT_ARG_LIST_LENGTH;
    int arg_count = 0;
    enum token_type seperators[] = {seperator, end_of_exp};

    // iterates until end_of_exp lexeme is reached
    while (parser->lexeme_list->list[parser->token_ptr - 1]->type != end_of_exp)
    {
        struct expression_node *tmp = parse_expression(parser, NULL, NULL, seperators, 2);

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
    Parser *parser,
    struct expression_component *parent,
    int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;
    assert(rec_lvl >= 0);

    // Base cases
    if (list[parser->token_ptr]->type == END_OF_FILE)
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
                parent->type == UNKNOWN_FUNC) &&
            list[parser->token_ptr]->type != OPEN_PARENTHESIS &&
            list[parser->token_ptr]->type != OPEN_SQUARE_BRACKETS &&
            list[parser->token_ptr]->type != ATTRIBUTE_ARROW)
        {
            return parent;
        }
    }

    // skips token if needed
    if (list[parser->token_ptr]->type == ATTRIBUTE_ARROW)
        parser->token_ptr++;

    struct expression_component *component = malloc_expression_component();

    // Parse list constants
    if (rec_lvl == 0 && !parent && list[parser->token_ptr]->type == OPEN_SQUARE_BRACKETS)
    {
        component->type = LIST_CONSTANT;
        parser->token_ptr++;
        struct expression_node **elements = parse_expressions_by_seperator(parser, COMMA, CLOSING_SQUARE_BRACKETS);
        component->meta_data.list_const.list_elements = elements;
        component->meta_data.list_const.list_length = get_expression_list_length(elements);
    }

    // Parses inline declared functions
    else if (get_keyword_type(list[parser->token_ptr]->ident) == FUNC)
    {
        component->type = UNKNOWN_FUNC;
        component->meta_data.inline_func = parse_inline_func(parser, rec_lvl);
    }
    else if (list[parser->token_ptr]->type == NUMERIC_LITERAL)
    {
        component->type = NUMERIC_CONSTANT;
        if (is_numeric_const_fractional(parser, parser->token_ptr))
        {
            component->meta_data.numeric_const = compute_fractional_double(list[parser->token_ptr], list[parser->token_ptr + 2]);
            parser->token_ptr += 2;
        }
        else
        {
            component->meta_data.numeric_const = (double)atoi(list[parser->token_ptr]->ident);
        }
        parser->token_ptr++;
        // strings
    }
    else if (list[parser->token_ptr]->type == STRING_LITERALS)
    {
        component->type = STRING_CONSTANT;
        component->meta_data.string_literal = malloc_string_cpy(list[parser->token_ptr]->ident);
        parser->token_ptr++;

        // some variable reference
    }
    else if (list[parser->token_ptr]->type == IDENTIFIER)
    {
        component->type = VARIABLE;
        component->meta_data.variable_reference = malloc_string_cpy(list[parser->token_ptr]->ident);
        parser->token_ptr++;

        // list index
    }
    else if (list[parser->token_ptr]->type == OPEN_SQUARE_BRACKETS)
    {
        component->type = LIST_INDEX;
        parser->token_ptr++;
        enum token_type end_of_exp[] = {CLOSING_SQUARE_BRACKETS};
        component->meta_data.list_index = parse_expression(parser, NULL, NULL, end_of_exp, 1);

        // function call
    }
    else if (list[parser->token_ptr]->type == OPEN_PARENTHESIS)
    {
        component->type = FUNC_CALL;
        parser->token_ptr++;
        struct expression_node **args = parse_expressions_by_seperator(parser, COMMA, CLOSING_PARENTHESIS);
        component->meta_data.func_data.func_args = args;
        component->meta_data.func_data.args_num = get_expression_list_length(args);
    }
    else
    {
        // TODO -> Invalid token error, handle this
        free(component);
        return NULL;
    }

    component->sub_component = parent;

    return parse_expression_component(parser, component, ++rec_lvl);
}

// uses reverse polish notation
struct expression_node *parse_expression(
    Parser *parser,
    struct expression_node *LHS,
    struct expression_node *RHS,
    enum token_type ends_of_exp[],
    const int ends_of_exp_length)
{
    assert(parser);

    struct token **list = parser->lexeme_list->list;

    if (list[parser->token_ptr]->type == END_OF_FILE)
        return LHS;

    // Base case (meet end of expression token)
    if (is_lexeme_in_list(list[parser->token_ptr]->type, ends_of_exp, ends_of_exp_length))
    {
        parser->token_ptr++;
        return LHS;
    }

    // Handles and Computes sub expressions
    if (list[parser->token_ptr]->type == OPEN_PARENTHESIS)
    {
        parser->token_ptr++;
        enum token_type end_of_exp[] = {CLOSING_PARENTHESIS};
        struct expression_node *sub_exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

        if (!LHS)
            return parse_expression(parser, sub_exp, RHS, ends_of_exp, ends_of_exp_length);
        else
            return parse_expression(parser, LHS, sub_exp, ends_of_exp, ends_of_exp_length);
    }

    struct expression_node *node = malloc_expression_node();

    if (is_lexeme_preliminary_expression_token(list[parser->token_ptr]))
    {

        node->component = parse_expression_component(parser, NULL, 0);
        node->type = VALUE;

        if (!LHS)
            return parse_expression(parser, node, RHS, ends_of_exp, ends_of_exp_length);
        else
            return parse_expression(parser, LHS, node, ends_of_exp, ends_of_exp_length);
    }

    // Handles ALL math/bitwise/logical operators
    switch (list[parser->token_ptr]->type)
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

    parser->token_ptr++;

    return parse_expression(parser, node, NULL, ends_of_exp, ends_of_exp_length);
}

/* Mallocs abstract syntac tree node */
AST_node *malloc_ast_node()
{
    AST_node *node = malloc(sizeof(AST_node));
    node->body = NULL;
    node->prev = NULL;
    node->next = NULL;
    node->type = -1;
    node->line_num = -1;
    return node;
}

/* The function ONLY frees the ast_node itself, not the other nodes that its pointing to */
void free_ast_node(AST_node *node)
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

    case INLINE_FUNCTION_DECLARATION:
    {
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
    AST_node *ptr = list->head;

    // frees all ast_nodes in list
    while (ptr)
    {
        AST_node *tmp = ptr->next;
        free_ast_node(ptr);
        ptr = tmp;
    }

    free(list);
}

/*
Adds a ast_node to the high abstraction ast_list data structure (i.e linked list)
NOTE: list is volatile, and MUST stay that way to prevent aggressive compiler optimizations from breaking crucial logic
*/

void push_to_ast_list(volatile struct ast_list *list, AST_node *node)
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
AST_node *parse_variable_declaration(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    // Sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == LET);
    assert(list[parser->token_ptr + 1]->type == IDENTIFIER && list[parser->token_ptr + 2]->type == ASSIGNMENT_OP);

    AST_node *node = malloc_ast_node();

    node->type = VAR_DECLARATION;
    node->line_num = list[parser->token_ptr]->line_num;
    // gets the var name
    node->identifier.declared_var = malloc_string_cpy(list[parser->token_ptr + 1]->ident);

    // parses its assignment value (i.e an expression)
    parser->token_ptr += 3;
    enum token_type end_of_exp[] = {SEMI_COLON};
    node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

    return node;
}

/* Creates AST node and parses while loop block */
AST_node *parse_while_loop(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == WHILE);
    assert(list[parser->token_ptr + 1]->type == OPEN_PARENTHESIS);

    AST_node *node = malloc_ast_node();

    node->type = WHILE_LOOP;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr += 2;

    // parse conditional expression
    enum token_type end_of_exp[] = {CLOSING_PARENTHESIS};
    node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

    // recursively parses inner code block
    assert(list[parser->token_ptr]->type == OPEN_CURLY_BRACKETS);
    parser->token_ptr++;
    enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/* Create AST node and parses if conditional block */
AST_node *parse_if_conditional(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == IF);
    assert(list[parser->token_ptr + 1]->type == OPEN_PARENTHESIS);

    AST_node *node = malloc_ast_node();

    node->type = IF_CONDITIONAL;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr += 2;

    // parses conditional expression
    enum token_type end_of_exp[] = {CLOSING_PARENTHESIS};
    node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

    assert(list[parser->token_ptr]->type == OPEN_CURLY_BRACKETS);

    parser->token_ptr++;

    // recursively parses inner code block
    enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/* Parse loop termination (i.e break) */
AST_node *parse_loop_termination(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    assert(get_keyword_type(list[parser->token_ptr]->ident) == BREAK);
    assert(list[parser->token_ptr + 1]->type == SEMI_COLON);

    AST_node *node = malloc_ast_node();

    node->type = LOOP_TERMINATOR;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr += 2;

    return node;
}

/* Parses loop continuation (i.e continue )*/
AST_node *parse_loop_continuation(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    assert(get_keyword_type(list[parser->token_ptr]->ident) == CONTINUE);
    assert(list[parser->token_ptr + 1]->type == SEMI_COLON);

    AST_node *node = malloc_ast_node();

    node->type = LOOP_CONTINUATION;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr += 2;

    return node;
}

/* Parses return expression (i.e return keyword )*/
AST_node *parse_return_expression(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    assert(get_keyword_type(list[parser->token_ptr]->ident) == RETURN);

    AST_node *node = malloc_ast_node();

    node->type = RETURN_VAL;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr++;
    enum token_type end_of_exp[] = {SEMI_COLON};

    node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

    return node;
}

/* Creates AST node and parse else (or else if) conditional block */
AST_node *parse_else_conditional(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == ELSE);
    assert(get_keyword_type(list[parser->token_ptr + 1]->ident) == IF || list[parser->token_ptr + 1]->type == OPEN_CURLY_BRACKETS);

    // else if block
    if (get_keyword_type(list[parser->token_ptr + 1]->ident) == IF)
    {
        AST_node *node = malloc_ast_node();

        node->type = ELSE_IF_CONDITIONAL;
        node->line_num = list[parser->token_ptr]->line_num;

        // parse conditional expression
        assert(list[parser->token_ptr + 2]->type == OPEN_PARENTHESIS);
        enum token_type end_of_exp[] = {CLOSING_PARENTHESIS};
        parser->token_ptr += 3;
        node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

        // recursively parses inner code block
        assert(list[parser->token_ptr]->type == OPEN_CURLY_BRACKETS);
        parser->token_ptr++;
        enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
        node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

        return node;
        // regular else block
    }
    else if (list[parser->token_ptr + 1]->type == OPEN_CURLY_BRACKETS)
    {
        AST_node *node = malloc_ast_node();

        node->type = ELSE_CONDITIONAL;
        node->line_num = list[parser->token_ptr]->line_num;

        enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
        parser->token_ptr += 2;
        node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

        return node;
    }
    else
    {
        // bad syntax should not happen
        printf("Wrong syntax for else conditional at line: %d\n", list[parser->token_ptr]->line_num);
        return NULL;
    }

    return NULL;
}

/* Creates AST node and parses function declaration block */
AST_node *parse_func_declaration(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == FUNC);
    assert(list[parser->token_ptr + 1]->type == IDENTIFIER);
    assert(list[parser->token_ptr + 2]->type == OPEN_PARENTHESIS);

    AST_node *node = malloc_ast_node();

    node->type = FUNCTION_DECLARATION;
    node->line_num = list[parser->token_ptr]->line_num;

    node->identifier.func_name = malloc_string_cpy(list[parser->token_ptr + 1]->ident);
    parser->token_ptr += 3;

    // parses function args
    struct expression_node **args = parse_expressions_by_seperator(parser, COMMA, CLOSING_PARENTHESIS);
    node->ast_data.func_args.func_prototype_args = args;
    node->ast_data.func_args.args_num = get_expression_list_length(args);

    // parses function inner code block
    assert(list[parser->token_ptr]->type == OPEN_CURLY_BRACKETS);
    enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    parser->token_ptr++;
    node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/*
Parses inline function syntax, very similar to regular function declaration parsing.
But not have corresponding identifer */
AST_node *parse_inline_func(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == FUNC);
    assert(list[parser->token_ptr + 1]->type == OPEN_PARENTHESIS);

    AST_node *node = malloc_ast_node();

    node->type = INLINE_FUNCTION_DECLARATION;
    node->line_num = list[parser->token_ptr]->line_num;
    node->identifier.func_name = NULL;

    parser->token_ptr += 2;

    // parses function args
    struct expression_node **args = parse_expressions_by_seperator(parser, COMMA, CLOSING_PARENTHESIS);
    node->ast_data.func_args.func_prototype_args = args;
    node->ast_data.func_args.args_num = get_expression_list_length(args);

    // parses function inner code block
    assert(list[parser->token_ptr]->type == OPEN_CURLY_BRACKETS);
    enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    parser->token_ptr++;
    node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

    // Remark: parser->token_ptr points to next token after end_of_block[] token
    return node;
}

/* Parses variable assignment or expression component (i.e function call) */
AST_node *parse_variable_assignment(Parser *parser, int rec_lvl)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    assert(is_lexeme_preliminary_expression_token(list[parser->token_ptr]));

    struct expression_component *component = parse_expression_component(parser, NULL, 0);

    assert(list[parser->token_ptr]->type == SEMI_COLON || list[parser->token_ptr]->type == ASSIGNMENT_OP);

    AST_node *node = malloc_ast_node();
    node->identifier.expression_component = component;
    node->line_num = list[parser->token_ptr]->line_num;

    if (list[parser->token_ptr]->type == SEMI_COLON)
    {
        node->type = EXPRESSION_COMPONENT;
        parser->token_ptr++;
    }
    else if (list[parser->token_ptr]->type == ASSIGNMENT_OP)
    {
        node->type = VAR_ASSIGNMENT;
        parser->token_ptr++;
        enum token_type end_of_exp[] = {SEMI_COLON};
        node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);
    }

    return node;
}

/* Recursively parses code block and returns a ast_node (abstract syntax tree node)*/
/* Function will always terminate with the parser->token_ptr pointing on the end exp token + 1*/
struct ast_list *parse_code_block(
    Parser *parser,
    AST_node *parent_block,
    int rec_lvl,
    enum token_type ends_of_block[],
    const int ends_of_block_length)
{
    assert(parser);
    struct token **list = parser->lexeme_list->list;

    if (is_lexeme_in_list(list[parser->token_ptr]->type, ends_of_block, ends_of_block_length))
    {
        parser->token_ptr++;
        return NULL;
    }

    struct ast_list *ast_list = malloc_ast_list();

    // loops until it encounters a end of block token
    while (!is_lexeme_in_list(list[parser->token_ptr]->type, ends_of_block, ends_of_block_length))
    {
        switch (get_keyword_type(list[parser->token_ptr]->ident))
        {

        // variable declaration
        case LET:
        {
            AST_node *node = parse_variable_declaration(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }
        // WHILE loop
        case WHILE:
        {
            AST_node *node = parse_while_loop(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // if conditional
        case IF:
        {
            AST_node *node = parse_if_conditional(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // else conditional
        case ELSE:
        {
            AST_node *node = parse_else_conditional(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // break (loop termination)
        case BREAK:
        {
            AST_node *node = parse_loop_termination(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // continue (loop continuation)
        case CONTINUE:
        {
            AST_node *node = parse_loop_continuation(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // return
        case RETURN:
        {
            AST_node *node = parse_return_expression(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // function declaration
        case FUNC:
        {
            AST_node *node = parse_func_declaration(parser, rec_lvl);
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
            if (is_lexeme_preliminary_expression_token(list[parser->token_ptr]))
            {
                AST_node *node = parse_variable_assignment(parser, rec_lvl);
                push_to_ast_list(ast_list, node);
            }
            else
            {
                // syntax error TODO
                printf("BAD TOKEN AT Line: %d\n", list[parser->token_ptr]->line_num);
                free_ast_list(ast_list);
                return NULL;
            }

            break;
        }
        }
    }

    ast_list->parent_block = parent_block;
    parser->token_ptr++;
    return ast_list;
}
