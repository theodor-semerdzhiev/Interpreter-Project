#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <setjmp.h>
#include <math.h>
#include "keywords.h"
#include "parser.h"
#include "errors.h"
#include "../generics/utilities.h"
#include "../generics/linkedlist.h"

/**
 * DESCRIPTION:
 * This array maps enums (i.e there int values) WITH a value representing its precedence
 * For example, VALUES is the only one with precedence 0
 * PLUS, MINUS, have precedence one and so one
 *
 * This function is crucial for expression parsing algorithm
 */

static short Precedence[25];

/**
 * DESCRIPTION:
 * Should always be called before parsing to initialize the precedence table
 */
void init_Precedence()
{
    Precedence[VALUE] = 0;

    Precedence[PLUS] = 1;
    Precedence[MINUS] = 1;

    Precedence[MULT] = 2;
    Precedence[DIV] = 2;
    Precedence[MOD] = 2;
    Precedence[BITWISE_AND] = 2;
    Precedence[BITWISE_OR] = 2;
    Precedence[BITWISE_XOR] = 2;
    Precedence[SHIFT_LEFT] = 2;
    Precedence[SHIFT_RIGHT] = 2;

    Precedence[EXPONENT] = 3;

    Precedence[LOGICAL_AND] = 4;
    Precedence[LOGICAL_OR] = 4;

    Precedence[GREATER_THAN] = 5;
    Precedence[GREATER_EQUAL] = 5;
    Precedence[LESSER_THAN] = 5;
    Precedence[LESSER_EQUAL] = 5;
    Precedence[EQUAL_TO] = 5;

}

/** DESCRIPTION:
 * Makes a long jump and returns to when the parsing function was called
 * Skipping over all the recursive nesting
 *
 * PARAMS:
 * parser: parser instance, used for getting the jump buffer
 *
 * */
static void stop_parsing(Parser *parser)
{
    parser->error_indicator = true;
    longjmp(*parser->error_handler, 1);
}

/**
 * DESCRIPTION:
 * Parses access modifers, ["private" | "global" ] [KEYWORD]
 *
 * PARAMS:
 * parser: parser instance
 * next_keyword: what keyword the access modifer keyword should followed by
 * */
static AccessModifier parse_access_modifer(Parser *parser, KeywordType next_keyword)
{
    assert(parser);

    Token **list = parser->token_list->list;
    if (list[parser->token_ptr]->type != KEYWORD)
    {
        parser->error_indicator = true;
        print_expected_token_err(parser, "Access Modifer", true, NULL);
        stop_parsing(parser);
        return DOES_NOT_APPLY;
    }

    if (list[parser->token_ptr]->type == KEYWORD &&
        get_keyword_type(list[parser->token_ptr]->ident) != next_keyword)
    {

        switch (get_keyword_type(list[parser->token_ptr]->ident))
        {
        case GLOBAL_KEYWORD:
            parser->token_ptr++;
            return GLOBAL_ACCESS;

        case PRIVATE_KEYWORD:
            parser->token_ptr++;
            return PRIVATE_ACCESS;

        default:
        {
            parser->error_indicator = true;
            print_expected_token_err(parser, "Access Modifer", true, NULL);
            stop_parsing(parser);
            return DOES_NOT_APPLY;
        }
        }
    }

    return PUBLIC_ACCESS;
}

/**
 * DESCRIPTION:
 * Useful function for skipping recurrent tokens
 *
 * PARAMS:
 * parser: parser instance
 * type: token type that should be skipped
 */
static void skip_recurrent_tokens(Parser *parser, enum token_type type)
{
    Token **list = parser->token_list->list;
    while (list[parser->token_ptr]->type == type)
        parser->token_ptr++;
}

/**
 * DESCRIPTION:
 * Initializes parser struct
 *
 * DEFAULTS:
 * error_indicator: false
 * token_list: NULL
 * memtracker: creates new intance
 * token_ptr: 0
 * file_name: NULL
 * ctx: REGULAR_CTX
 * error_handler: NULL
 *
 * lines_count: 0
 * lines: NULL
 *
 * NOTE:
 * Returns NULL if malloc returns null
 */
Parser *init_Parser()
{
    Parser *parser = malloc(sizeof(Parser));
    if (!parser)
        return NULL;
    parser->error_indicator = false;
    parser->token_list = NULL;
    parser->memtracker = init_memtracker();
    if (!parser->memtracker)
    {
        free(parser);
        return NULL;
    }
    parser->token_ptr = 0;
    parser->file_name = NULL;
    parser->ctx = REGUALR_CTX;
    parser->error_handler = NULL;
    parser->lines.line_count = 0;
    parser->lines.lines = NULL;
    return parser;
}

/* Frees parser */
void free_parser(Parser *parser)
{
    if (!parser)
        return;

    free_token_list(parser->token_list);
    free_memtracker_without_pointers(parser->memtracker);
    free(parser->file_name);

    for (int i = 0; i < parser->lines.line_count; i++)
        free(parser->lines.lines[i]);

    free(parser->lines.lines);

    free(parser);
}

/* index should point to the first occurence of the numeric constant */
bool is_numeric_const_fractional(Parser *parser, int index)
{
    Token **list = parser->token_list->list;
    return list[index]->type == NUMERIC_LITERAL &&
           list[++index]->type == DOT &&
           list[++index]->type == NUMERIC_LITERAL;
}

/* Checks if lexeme_type enum is in list */
bool is_lexeme_in_list(
    enum token_type type,
    const enum token_type list[],
    int list_length)
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
char *malloc_string_cpy(Parser *parser, const char *str)
{
    char *str_cpy = cpy_string(str);

    if (parser)
        push_to_memtracker(parser->memtracker, str_cpy, free);

    return str_cpy;
}

/* Computes fractional number (whole: integer part, frac: fractional part)*/
/* TODO: Fix issue where identifer is too long */
double compute_fractional_double(Token *whole, Token *frac)
{
    assert(whole->type == NUMERIC_LITERAL && frac->type == NUMERIC_LITERAL);
    double lhs = (double)atoi(whole->ident);

    double fraction = (double)atoi(frac->ident) / (pow(10, strlen(frac->ident)));
    return lhs + fraction;
}

/* Mallocs expression component struct */
ExpressionComponent *malloc_expression_component(Parser *parser)
{
    ExpressionComponent *component = malloc(sizeof(ExpressionComponent));
    component->sub_component = NULL;
    component->top_component = NULL;
    component->type = -1;
    component->token_num = parser ? parser->token_ptr : -1;

    if (parser)
        push_to_memtracker(parser->memtracker, component, free);

    return component;
}

/* Mallocs malloc_exp_node setting all fields to default values */
ExpressionNode *malloc_expression_node(Parser *parser)
{
    ExpressionNode *node = malloc(sizeof(ExpressionNode));
    node->LHS = NULL;
    node->RHS = NULL;
    node->component = NULL;
    node->negation = false;
    node->type = -1;
    node->token_num = parser->token_ptr;

    if (parser)
        push_to_memtracker(parser->memtracker, node, free);

    return node;
}

/* Checks wether lexeme_type is a valid beginning of expression component,
Is also valid for func keyword (inline functions syntax)
*/
bool is_preliminary_expression_token(Token *token)
{
    return token->type == IDENTIFIER ||
           token->type == OPEN_PARENTHESIS ||
           token->type == OPEN_SQUARE_BRACKETS ||
           token->type == NUMERIC_LITERAL ||
           token->type == STRING_LITERALS ||
           (token->type == KEYWORD && (get_keyword_type(token->ident) == FUNC_KEYWORD ||
                                       get_keyword_type(token->ident) == NULL_KEYWORD ||
                                       get_keyword_type(token->ident) == MAP_KEYWORD ||
                                       get_keyword_type(token->ident) == SET_KEYWORD));
}

/* Recursively frees expression component struct */
void free_expression_component(ExpressionComponent *component)
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
    case NULL_CONSTANT:
    {
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

    case INLINE_FUNC:
    {
        free_ast_node(component->meta_data.inline_func);

        free(component);
        return;
    }

    case HASHMAP_CONSTANT:
    {
        for (int i = 0; i < component->meta_data.HashMap.size; i++)
        {
            free_expression_tree(component->meta_data.HashMap.pairs[i]->key);
            free_expression_tree(component->meta_data.HashMap.pairs[i]->value);
            free(component->meta_data.HashMap.pairs[i]);
        }

        free(component->meta_data.HashMap.pairs);
        free(component);
        break;
    }

    case HASHSET_CONSTANT:
    {
        for (int i = 0; i < component->meta_data.HashSet.size; i++)
        {
            free_expression_tree(component->meta_data.HashSet.values[i]);
        }

        free(component->meta_data.HashSet.values);
        free(component);
        break;
    }
    }
}

/* Recursively frees binary expression parse tree */
void free_expression_tree(ExpressionNode *root)
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
ExpressionNode **parse_expressions_by_seperator(
    Parser *parser,
    enum token_type seperator,
    enum token_type end_of_exp)
{
    assert(parser);

    ExpressionNode **args = malloc(sizeof(ExpressionNode *) * DEFAULT_ARG_LIST_LENGTH);
    int arg_list_max_length = DEFAULT_ARG_LIST_LENGTH;
    int arg_count = 0;
    enum token_type seperators[] = {seperator, end_of_exp};

    // allows to update pointer if args is resized
    MallocNode *args_pointer = push_to_memtracker(parser->memtracker, args, free);

    // iterates until end_of_exp lexeme is reached
    while (parser->token_list->list[parser->token_ptr - 1]->type != end_of_exp)
    {
        // ExpressionNode *tmp = parse_expression(parser, NULL, NULL, seperators, 2);
        ExpressionNode *tmp = parse_expression(parser, seperators, 2);

        // if all argument tokens have been consumed
        if (!tmp)
            break;

        args[arg_count++] = tmp;

        if (arg_list_max_length == arg_count)
        {
            arg_list_max_length *= 2;
            ExpressionNode **new_arg_list = malloc(sizeof(ExpressionNode *) * arg_list_max_length);

            for (int i = 0; i < arg_count; i++)
                new_arg_list[i] = args[i];

            free(args);

            args_pointer->ptr = new_arg_list;
            args = new_arg_list;
        }
    }

    args[arg_count] = NULL;
    return args;
}

/* Parses map syntax */
KeyValue **parser_key_value_pair_exps(
    Parser *parser,
    enum token_type key_val_seperators,
    enum token_type pair_seperators,
    enum token_type end_of_exp)
{
    KeyValue **pairs = malloc(sizeof(KeyValue) * DEFAULT_ARG_LIST_LENGTH);
    int pair_list_max_length = DEFAULT_ARG_LIST_LENGTH;
    int pair_count = 0;

    enum token_type seperators1[] = {key_val_seperators};
    enum token_type seperators2[] = {pair_seperators, end_of_exp};

    // allows to update pointer if args is resized
    MallocNode *args_pointer = push_to_memtracker(parser->memtracker, pairs, free);

    // iterates until end_of_exp lexeme is reached
    while (parser->token_list->list[parser->token_ptr - 1]->type != end_of_exp)
    {
        // ExpressionNode *key = parse_expression(parser, NULL, NULL, seperators1, 1);
        // ExpressionNode *value = parse_expression(parser, NULL, NULL, seperators2, 2);

        ExpressionNode *key = parse_expression(parser, seperators1, 1);
        ExpressionNode *value = parse_expression(parser, seperators2, 2);

        KeyValue *pair = malloc(sizeof(KeyValue));
        pair->key = key;
        pair->value = value;

        push_to_memtracker(parser->memtracker, pair, free);

        pairs[pair_count++] = pair;

        if (pair_list_max_length == pair_count)
        {
            pair_list_max_length *= 2;
            KeyValue **new_pair_list = malloc(sizeof(KeyValue *) * pair_list_max_length);

            for (int i = 0; i < pair_count; i++)
                new_pair_list[i] = pairs[i];

            free(pairs);

            args_pointer->ptr = new_pair_list;
            pairs = new_pair_list;
        }
    }

    pairs[pair_count] = NULL;
    return pairs;
}

/*
This function is responsible for recursively parsing expression components (i.e num in -> num 10 *)
rec_lvl (recursion level) should be called with inital value of 0
NUMERIC_CONSTANT -> number
STRING_CONSTANT -> string
LIST_CONSTANT -> list
FUNC_CALL -> func call -> (arg1, ..., arg_n)
LIST_INDEX -> [...]

Note: This function does not check for a end of expression token, it knows when to end the recursion
*/
ExpressionComponent *parse_expression_component(
    Parser *parser,
    ExpressionComponent *parent,
    int rec_lvl)
{
    assert(parser);
    assert(rec_lvl >= 0);

    Token **list = parser->token_list->list;

    // Base cases
    if (list[parser->token_ptr]->type == END_OF_FILE)
    {
        print_unexpected_end_of_file_err(parser, NULL);
        stop_parsing(parser);
        return parent;
    }

    if (parent)
    {

        if (

            list[parser->token_ptr]->type != OPEN_PARENTHESIS &&
            list[parser->token_ptr]->type != OPEN_CURLY_BRACKETS &&
            list[parser->token_ptr]->type != OPEN_SQUARE_BRACKETS &&
            list[parser->token_ptr]->type != ATTRIBUTE_ARROW)
        {
            return parent;
        }
    }

    bool preceded_by_arrow = false;

    // skips token if needed
    if (list[parser->token_ptr]->type == ATTRIBUTE_ARROW)
    {
        parser->token_ptr++;
        preceded_by_arrow = true;
    }

    ExpressionComponent *component = malloc_expression_component(parser);
    component->line_num = list[parser->token_ptr]->line_num;

    if (get_keyword_type(list[parser->token_ptr]->ident) == MAP_KEYWORD)
    {
        component->type = HASHMAP_CONSTANT;
        if (list[parser->token_ptr + 1]->type != OPEN_CURLY_BRACKETS)
        {
            parser->token_ptr++;
            print_expected_token_err(parser, "Open Curly Brackets ('{')", false,
                                     "Proper Syntax: map { v1 : e1, v2: e2, ... };");
            stop_parsing(parser);
            return NULL;
        }

        parser->token_ptr += 2;
        ParsingContext tmp = parser->ctx;
        parser->ctx = MAP_CTX;

        KeyValue **keyval_pairs = parser_key_value_pair_exps(parser, COLON, COMMA, CLOSING_CURLY_BRACKETS);
        component->meta_data.HashMap.pairs = keyval_pairs;
        component->meta_data.HashMap.size = get_pointer_list_length((void **)keyval_pairs);

        parser->ctx = tmp;
    }
    else if (get_keyword_type(list[parser->token_ptr]->ident) == SET_KEYWORD)
    {
        component->type = HASHSET_CONSTANT;
        if (list[parser->token_ptr + 1]->type != OPEN_CURLY_BRACKETS)
        {
            parser->token_ptr++;
            print_expected_token_err(parser, "Open Curly Brackets ('{')", false,
                                     "Proper Syntax: set { e1, e2, ... };");
            stop_parsing(parser);
            return NULL;
        }
        parser->token_ptr += 2;

        ParsingContext tmp = parser->ctx;
        parser->ctx = SET_CTX;

        ExpressionNode **elements = parse_expressions_by_seperator(parser, COMMA, CLOSING_CURLY_BRACKETS);
        component->meta_data.HashSet.values = elements;
        component->meta_data.HashSet.size = get_pointer_list_length((void **)elements);

        parser->ctx = tmp;
    }
    // void pointer (null pointer)
    else if (get_keyword_type(list[parser->token_ptr]->ident) == NULL_KEYWORD)
    {
        component->type = NULL_CONSTANT;
        parser->token_ptr++;
    }

    // Parse list constants (Cannot have parent component)
    else if (rec_lvl == 0 && !parent && list[parser->token_ptr]->type == OPEN_SQUARE_BRACKETS)
    {
        component->type = LIST_CONSTANT;

        ParsingContext tmp = parser->ctx;
        parser->ctx = LIST_CTX;

        parser->token_ptr++;
        ExpressionNode **elements = parse_expressions_by_seperator(parser, COMMA, CLOSING_SQUARE_BRACKETS);
        component->meta_data.list_const.list_elements = elements;
        component->meta_data.list_const.list_length = get_pointer_list_length((void **)elements);

        parser->ctx = tmp;
    }

    // Parses inline declared functions (Cannot have parent component)
    else if (rec_lvl == 0 && !parent && get_keyword_type(list[parser->token_ptr]->ident) == FUNC_KEYWORD)
    {
        component->type = INLINE_FUNC;
        component->meta_data.inline_func = parse_inline_func(parser, rec_lvl);
    }
    // numeric constants (Cannot have parent component)
    else if (rec_lvl == 0 && !parent && list[parser->token_ptr]->type == NUMERIC_LITERAL)
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
    }
    // string constants (Cannot have parent component)
    else if (rec_lvl == 0 && !parent && list[parser->token_ptr]->type == STRING_LITERALS)
    {
        component->type = STRING_CONSTANT;
        component->meta_data.string_literal = malloc_string_cpy(parser, list[parser->token_ptr]->ident);
        parser->token_ptr++;
    }
    // some variable reference
    else if (list[parser->token_ptr]->type == IDENTIFIER)
    {
        component->type = VARIABLE;
        component->meta_data.variable_reference = malloc_string_cpy(parser, list[parser->token_ptr]->ident);
        parser->token_ptr++;
    }
    // list index (Cannot have been preceded by an attribute arrow)
    else if (list[parser->token_ptr]->type == OPEN_SQUARE_BRACKETS &&
             parser->token_ptr > 0 &&
             list[parser->token_ptr - 1]->type != ATTRIBUTE_ARROW)
    {
        component->type = LIST_INDEX;
        parser->token_ptr++;
        enum token_type end_of_exp[] = {CLOSING_SQUARE_BRACKETS};
        component->meta_data.list_index = parse_expression(parser, end_of_exp, 1);
        // component->meta_data.list_index = parse_expression(parser, NULL, NULL, end_of_exp, 1);
    }
    // function call
    else if (rec_lvl > 0 && !preceded_by_arrow && list[parser->token_ptr]->type == OPEN_PARENTHESIS)
    {
        // SYNTAX ERROR: INVALID USE OF AN ARROW TOKEN
        if (parser->token_ptr > 0 && list[parser->token_ptr - 1]->type == ATTRIBUTE_ARROW)
        {
            parser->error_indicator = true;
            print_invalid_token_err(parser,
                                    "Invalid use of Arrow operator. Did you intend a function call?\n"
                                    "Example: [Identifier] (args, ...)");
            stop_parsing(parser);
            return NULL;
        }

        component->type = FUNC_CALL;
        parser->token_ptr++;
        ExpressionNode **args = parse_expressions_by_seperator(parser, COMMA, CLOSING_PARENTHESIS);
        component->meta_data.func_data.func_args = args;
        component->meta_data.func_data.args_num = get_pointer_list_length((void **)args);
    }
    else
    {
        // gives a tailored error messages
        switch (list[parser->token_ptr]->type)
        {
        case OPEN_SQUARE_BRACKETS:
            print_invalid_expression_component(parser,
                                               "Statically defined list cannot have parent component (i.e '->')\n"
                                               "Proper Syntax: [e1, e2, ... ] -> ...");
            break;
        case NUMERIC_LITERAL:
            print_invalid_expression_component(parser,
                                               "Numeric constant cannot have parent component (i.e '->')");
            break;
        case STRING_LITERALS:
            print_invalid_expression_component(parser,
                                               "Statically defined String cannot have parent component (i.e '->')\n"
                                               "Proper Syntax: \"abcde ... \" -> ... ");
            break;
        case FUNC_KEYWORD:
            print_invalid_expression_component(parser,
                                               "Function/Inline Function cannot have parent component (i.e '->')\n"
                                               "Proper Syntax:\n"
                                               "func ( ... ) { ... } -> ...\n"
                                               "func function ( ... ) { ... } -> ...");
            break;
        default:
            print_invalid_token_err(parser, "An expression component was expected");
        }

        stop_parsing(parser);
        return NULL;
    }

    component->sub_component = parent;
    if (parent)
        parent->top_component = component;

    return parse_expression_component(parser, component, ++rec_lvl);
}

/**
 * DESCRIPTION:
 * Checks whether a token type is an operator token
 */
bool isOpToken(enum token_type type)
{
    switch (type)
    {
    case MULT_OP:
    case MINUS_OP:
    case PLUS_OP:
    case DIV_OP:
    case MOD_OP:
    case EXPONENT_OP:
    case GREATER_THAN_OP:
    case GREATER_EQUAL_OP:
    case LESSER_THAN_OP:
    case LESSER_EQUAL_OP:
    case EQUAL_TO_OP:
    case LOGICAL_AND_OP:
    case LOGICAL_OR_OP:
    case BITWISE_AND_OP:
    case BITWISE_OR_OP:
    case BITWISE_XOR_OP:
    case SHIFT_LEFT_OP:
    case SHIFT_RIGHT_OP:
    case LOGICAL_NOT_OP:
        return true;

    default:
        return false;
    }
}

/**
 * DESCRIPTION:
 * Converts token type to an expression node type
 */
static enum expression_token_type convertOpTokenToOp(enum token_type type)
{
    switch (type)
    {
    case MULT_OP:
        return MULT;
    case MINUS_OP:
        return MINUS;
    case PLUS_OP:
        return PLUS;
    case DIV_OP:
        return DIV;
    case MOD_OP:
        return MOD;
    case EXPONENT_OP:
        return EXPONENT;
    case GREATER_THAN_OP:
        return GREATER_THAN;
    case GREATER_EQUAL_OP:
        return GREATER_EQUAL;
    case LESSER_THAN_OP:
        return LESSER_THAN;
    case LESSER_EQUAL_OP:
        return LESSER_EQUAL;
    case EQUAL_TO_OP:
        return EQUAL_TO;
    case LOGICAL_AND_OP:
        return LOGICAL_AND;
    case LOGICAL_OR_OP:
        return LOGICAL_OR;
    case BITWISE_AND_OP:
        return BITWISE_AND;
    case BITWISE_OR_OP:
        return BITWISE_OR;
    case BITWISE_XOR_OP:
        return BITWISE_XOR;
    case SHIFT_LEFT_OP:
        return SHIFT_LEFT;
    case SHIFT_RIGHT_OP:
        return SHIFT_RIGHT;

    default:
        return -1;
    }
}

/**
 * DESCRIPTION:
 * Wrapper for freeing linkedlist
 * Needed for memory tracker to match function type signature
 */
static void _free_LList(GenericLList *list) { GenericLList_free(list, false); }

#define precedence(type) Precedence[type] // macro for getting precedence of operators
#define endOfExp(type) is_lexeme_in_list(type, ends_of_exp, ends_of_exp_length)

/* Forward Delcaration */
static ExpressionNode *construct_expression_tree(GenericLList *inputList, GenericLList *opList);

/**
 * DESCRIPTION:
 * This function is responsible for parsing infix expressions
 * It uses a recursive descent approach where a ordered list of expression components and operators are collected
 * And, from the top-down, the expression is recursively constructed
 *
 * PREVARIANT: the parser instance token pointer MUST point to first token of the expression
 *
 * POSTVARIANT: the parser instance token pointer MUST point to next token AFTER the end of expression token
 *
 * PARAMS:
 * parser: instance of the parser
 * ends_of_exp: the list of possible tokens that represent then end of the expression
 * ends_of_exp_length: the length of the array above
 */
ExpressionNode *parse_expression(
    Parser *parser,
    const enum token_type ends_of_exp[],
    const int ends_of_exp_length)
{
    Token **list = parser->token_list->list;

    // If we encounter a end of file token mid expression, this is NOT valid
    if (list[parser->token_ptr]->type == END_OF_FILE)
    {
        print_unexpected_end_of_file_err(parser, NULL);
        stop_parsing(parser);
    }

    // If expression is empty
    if (endOfExp(list[parser->token_ptr]->type))
    {
        parser->token_ptr++;
        return NULL;
    }

    GenericLList *inputList = init_GenericLList(NULL, free);
    if (!inputList)
        stop_parsing(parser);
    GenericLList *opList = init_GenericLList(NULL, free);
    if (!opList)
    {
        _free_LList(inputList);
        stop_parsing(parser);
    }

    push_to_memtracker(parser->memtracker, inputList, (void (*)(void *))_free_LList);
    push_to_memtracker(parser->memtracker, opList, (void (*)(void *))_free_LList);

    // this loop populates the inputList and the opList
    while (
        list[parser->token_ptr]->type != END_OF_FILE &&
        !endOfExp(list[parser->token_ptr]->type))
    {
        bool is_mult_minus_1 = false; // handles numeric negation like -(10 +20)
        bool is_exp_negated = false;  // handles ! logical not operator

        // handles multiple negation operator (i.e '!')
        while (list[parser->token_ptr]->type == MINUS_OP)
        {
            is_mult_minus_1 = !is_mult_minus_1;
            parser->token_ptr++;
        }

        // handles multiple negation operator (i.e '!')
        while (list[parser->token_ptr]->type == LOGICAL_NOT_OP)
        {
            is_exp_negated = !is_exp_negated;
            parser->token_ptr++;
        }

        ExpressionNode *leaf = NULL;

        // Case 1: Sub Expression
        if (list[parser->token_ptr]->type == OPEN_PARENTHESIS)
        {
            parser->token_ptr++;
            enum token_type end_of_exp[] = {CLOSING_PARENTHESIS};
            leaf = parse_expression(parser, end_of_exp, 1);

            // Case 2: Expression component
        }
        else if (is_preliminary_expression_token(list[parser->token_ptr]))
        {

            ExpressionComponent *exp_component = parse_expression_component(parser, NULL, 0);
            leaf = malloc_expression_node(parser);
            leaf->type = VALUE;
            leaf->component = exp_component;

            // we do not have a valid expression component
        }
        else
        {

            print_missing_exp_component_err(parser,
                                            "Unexpected end of expression. Expected expression component.");
            stop_parsing(parser);
            return NULL;
        }

        // handles numerical negation by multiplying by -1
        if (is_mult_minus_1)
        {
            ExpressionNode *negative_1 = malloc_expression_node(parser);
            negative_1->type = VALUE;
            negative_1->component = malloc_expression_component(parser);
            negative_1->component->type = NUMERIC_CONSTANT;
            negative_1->component->meta_data.numeric_const = -1;

            ExpressionNode *mult_node = malloc_expression_node(parser);
            mult_node->type = MULT;
            mult_node->LHS = leaf;
            mult_node->RHS = negative_1;
            leaf = mult_node;
        }

        // if encounter a end of file token, means an error
        if (list[parser->token_ptr]->type == END_OF_FILE)
        {
            print_unexpected_end_of_file_err(parser, NULL);
            stop_parsing(parser);
        }

        leaf->negation = is_exp_negated;

        // adds Expression node to input list
        GenericLList_addLast(inputList, leaf);

        // if we encounter a end of expression after expression component,
        // then it MUST be the end of the expression
        if (endOfExp(list[parser->token_ptr]->type))
            break;

        ExpressionNode *oproot = NULL;

        // if we encountered a leaf, and its not the end of the expression yet,
        // then we MUST have an operator
        if (isOpToken(list[parser->token_ptr]->type))
        {
            oproot = malloc_expression_node(parser);
            oproot->type = convertOpTokenToOp(list[parser->token_ptr]->type);
            parser->token_ptr++;
        }
        else
        {
            print_missing_operator_err(parser, NULL);
            parser->error_indicator = true;
            stop_parsing(parser);
            return NULL;
        }

        // adds operator to list
        GenericLList_addLast(opList, oproot);

        if (list[parser->token_ptr]->type == END_OF_FILE)
        {
            print_unexpected_end_of_file_err(parser, NULL);
            stop_parsing(parser);
            return NULL;
        }

        // an operator MUST be followed by an other expression component
        // or a minus sign, if its a negative number
        if (!is_preliminary_expression_token(list[parser->token_ptr]) &&
            list[parser->token_ptr]->type != LOGICAL_NOT_OP &&
            list[parser->token_ptr]->type != MINUS_OP)
        {
            parser->error_indicator = true;
            print_missing_exp_component_err(
                parser,
                "Unexpected end of expression, expected expression component.");
            stop_parsing(parser);
        }
    }

    // If we encounter a end of file token mid expression, this is NOT valid expression
    if (list[parser->token_ptr]->type == END_OF_FILE)
    {
        print_unexpected_end_of_file_err(parser, NULL);
        stop_parsing(parser);
        return NULL;
    }

    parser->token_ptr++;

    /**
     * ASSUMPTIONS:
     * If we got to this point, then we should have a valid expression
     * Therefor, # of operators = # number of leafs  - 1
     */

    // Using the operator Stack and input List, we recursively construct the tree
    ExpressionNode *root = construct_expression_tree(inputList, opList);
    assert(inputList->length == 0 && opList->length == 0);

    // free lists
    remove_ptr_from_memtracker(parser->memtracker, inputList, true);
    remove_ptr_from_memtracker(parser->memtracker, opList, true);
    return root;
}

/**
 * DESCRIPTION:
 * This a type of recursive descent parser for constructing the expression tree from the list of operands (operators) and inputs (i.e expression components),
 * properly ordered by position in the expression
 * Example: (A * B + C) ==> Inputs: [A,B,C], Operands: [*, +]
 *
 * The algorithm relies on the following invariant:
 *      length of inputList - 1 = length of opStack
 *
 * This is true for any valid expression, i.e the number of operators == the number of inputs - 1
 * For each recursive case, given the initial inputs are valid, this invariant is always maintained
 * Proof: Case 3,4 both pop an equal number of elements from input and operator lists
 *
 * When the algorithm reaches a base case, # of inputs popped = # of outputs popped + 1
 * Thereby, making sure both lists are empty after the termination of the alg
 *
 * The correctness of this alg can self evidently be proved by induction
 *
 * PARAMS:
 * inputList: Properly ordered Linked list of all inputs to an expression (act as leafs)
 * opList: Properly ordered Linked list of all operators to an expression
 *
 */
static ExpressionNode *construct_expression_tree(GenericLList *inputList, GenericLList *opList)
{
    assert(inputList && opList);
    assert(inputList->length - 1 == opList->length);

    ExpressionNode *operator=(ExpressionNode *) GenericLList_popFirst(opList, false);
    const ExpressionNode *next_operator = (ExpressionNode *)GenericLList_head(opList);

    // Bases Cases

    // Case 1: Expression is a single expression component with no operators
    if (!operator)
    {
        return (ExpressionNode *)GenericLList_popFirst(inputList, false);
    }

    // Case 2: Expression is of the following format (A [op] B)
    if (!next_operator)
    {
        operator->LHS =(ExpressionNode *) GenericLList_popFirst(inputList, false);
        operator->RHS =(ExpressionNode *) GenericLList_popFirst(inputList, false);

        return operator;
    }

    // Recursive cases

    // Case 3: Expression as the following structure A [op1] B [op2] C,
    // where op1 > op2 in terms of precedence and A,B,C are leafs or sub expressions
    // Therefor current operator (op1) takes the first 2 expression components (A, B) as its children,
    // And then the next operator (op2) becomes the parent of current one (op1), and the rest of the expression (C)
    if (precedence(operator->type) >= precedence(next_operator->type))
    {
        operator->LHS =(ExpressionNode *) GenericLList_popFirst(inputList, false);
        operator->RHS =(ExpressionNode *) GenericLList_popFirst(inputList, false);

        ExpressionNode *next = (ExpressionNode *)GenericLList_popFirst(opList, false);
        next->LHS = operator;
        next->RHS = construct_expression_tree(inputList, opList);
        return next;

        // Case 4: Expression as the following structure: A [op1] B [op2] C,
        // where op1 <= op2 in terms of precedence and A,B,C are leafs or sub expressions
        // Therefor, current operator (op1) takes the first expression components(A) as its left child
        // and the sub expression (B [op2] C), as its right node
    }
    else
    {

        operator->LHS =(ExpressionNode *) GenericLList_popFirst(inputList, false);
        operator->RHS = construct_expression_tree(inputList, opList);
        return operator;
    }
}

/* Mallocs abstract syntac tree node */
AST_node *malloc_ast_node(Parser *parser)
{
    AST_node *node = malloc(sizeof(AST_node));
    node->body = NULL;
    node->prev = NULL;
    node->next = NULL;
    node->type = -1;
    node->access = DOES_NOT_APPLY;
    node->line_num = -1;
    node->token_num = parser->token_ptr;

    if (parser)
        push_to_memtracker(parser->memtracker, node, free);

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

    case OBJECT_DECLARATION:
    {
        for (int i = 0; i < node->ast_data.obj_args.args_num; i++)
            free_expression_tree(node->ast_data.obj_args.object_prototype_args[i]);

        free(node->ast_data.obj_args.object_prototype_args);

        free_ast_list(node->body);
        free(node->identifier.obj_name);
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
AST_List *malloc_ast_list(Parser *parser)
{
    AST_List *list = malloc(sizeof(AST_List));
    list->head = NULL;
    list->parent_block = NULL;
    list->tail = NULL;
    list->length = 0;

    if (parser)
        push_to_memtracker(parser->memtracker, list, free);

    return list;
}

/* Recursively frees ast_list struct */
void free_ast_list(AST_List *list)
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

void push_to_ast_list(AST_List *list, AST_node *node)
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
AST_node *parse_variable_declaration(Parser *parser, int rec_lvl __attribute__((unused)))
{
    assert(parser);
    Token **list = parser->token_list->list;

    AccessModifier access_modifier = parse_access_modifer(parser, LET_KEYWORD);

    // checks for identifier
    if (list[parser->token_ptr + 1]->type != IDENTIFIER)
    {
        parser->token_ptr++;
        print_expected_token_err(parser, "Variable Declaration Identifier", false,
                                 "Proper Syntax: let variable = ...");
        stop_parsing(parser);
        return NULL;
    }

    AST_node *node = malloc_ast_node(parser);
    node->access = access_modifier;
    node->type = VAR_DECLARATION;
    node->line_num = list[parser->token_ptr]->line_num;

    // gets the var name
    node->identifier.declared_var = malloc_string_cpy(parser, list[parser->token_ptr + 1]->ident);

    // if variable is not set to a value
    if (list[parser->token_ptr + 2]->type == SEMI_COLON)
    {
        node->ast_data.exp = NULL;
        parser->token_ptr += 3;
        return node;
    }

    // checks for assignment operator
    if (list[parser->token_ptr + 2]->type != ASSIGNMENT_OP)
    {
        char *identifier = list[parser->token_ptr + 1]->ident;
        char *tmp_str = malloc(sizeof(char) * (40 + strlen(identifier)));
        snprintf(tmp_str, 40 + strlen(identifier) + 1, "Proper Syntax: let %s = ... ;", identifier);
        parser->token_ptr += 2;
        print_expected_token_err(parser, "Assignment Operator ('=')", false, tmp_str);
        free(tmp_str);
        stop_parsing(parser);
        return NULL;
    }

    // parses its assignment value (i.e an expression)
    parser->token_ptr += 3;
    enum token_type end_of_exp[] = {SEMI_COLON};
    // node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);
    node->ast_data.exp = parse_expression(parser, end_of_exp, 1);

    skip_recurrent_tokens(parser, end_of_exp[0]);
    return node;
}

/* Creates AST node and parses while loop block */
AST_node *parse_while_loop(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->token_list->list;

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == WHILE_KEYWORD);

    // checks for parenthesis
    if (list[parser->token_ptr + 1]->type != OPEN_PARENTHESIS)
    {
        parser->token_ptr += 1;
        print_expected_token_err(parser, "Open Parenthesis ('(')", false,
                                 "Proper Syntax: while (...)");
        stop_parsing(parser);
        return NULL;
    }

    AST_node *node = malloc_ast_node(parser);

    node->type = WHILE_LOOP;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr += 2;

    // parse conditional expression
    enum token_type end_of_exp[] = {CLOSING_PARENTHESIS};
    // node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);
    node->ast_data.exp = parse_expression(parser, end_of_exp, 1);

    // checks for open curly brackets
    if (list[parser->token_ptr]->type != OPEN_CURLY_BRACKETS)
    {
        print_expected_token_err(parser, "Open Curly Brackets ('{')", false,
                                 "Proper Syntax: while (...) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    // recursively parses inner code block
    parser->token_ptr++;
    enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/* Create AST node and parses if conditional block */
AST_node *parse_if_conditional(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->token_list->list;

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == IF_KEYWORD);

    // checks for parenthesis
    if (list[parser->token_ptr + 1]->type != OPEN_PARENTHESIS)
    {
        parser->token_ptr += 1;
        print_expected_token_err(parser, "Open Parenthesis ('(')", false,
                                 "Proper Syntax: if (...) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    AST_node *node = malloc_ast_node(parser);

    node->type = IF_CONDITIONAL;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr += 2;

    // parses conditional expression
    enum token_type end_of_exp[] = {CLOSING_PARENTHESIS};
    node->ast_data.exp = parse_expression(parser, end_of_exp, 1);
    // node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

    // checks for open curly brackets
    if (list[parser->token_ptr]->type != OPEN_CURLY_BRACKETS)
    {
        print_expected_token_err(parser, "Open Curly Brackets ('{')", false,
                                 "Proper Syntax: if (...) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    parser->token_ptr++;

    // recursively parses inner code block
    enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/* Parse loop termination (i.e break) */
AST_node *parse_loop_termination(Parser *parser, int rec_lvl __attribute__((unused)))
{
    assert(parser);
    Token **list = parser->token_list->list;

    assert(get_keyword_type(list[parser->token_ptr]->ident) == BREAK_KEYWORD);

    // checks syntax
    if (list[parser->token_ptr + 1]->type != SEMI_COLON)
    {
        parser->token_ptr++;
        print_expected_token_err(parser, "Semicolon (';')", false,
                                 "Proper Syntax: break; \nbreak keyword is always standalone");
        stop_parsing(parser);
        return NULL;
    }

    AST_node *node = malloc_ast_node(parser);

    node->type = LOOP_TERMINATOR;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr += 2;

    skip_recurrent_tokens(parser, SEMI_COLON);

    return node;
}

/* Parses loop continuation (i.e continue )*/
AST_node *parse_loop_continuation(Parser *parser, int rec_lvl __attribute__((unused)))
{
    assert(parser);
    Token **list = parser->token_list->list;

    assert(get_keyword_type(list[parser->token_ptr]->ident) == CONTINUE_KEYWORD);

    // checks for parenthesis
    if (list[parser->token_ptr + 1]->type != SEMI_COLON)
    {
        parser->token_ptr += 1;
        print_expected_token_err(parser, "Semicolon (';')", false,
                                 "Proper Syntax: continue; \ncontinue Keyword is always standalone");
        stop_parsing(parser);
        return NULL;
    }

    AST_node *node = malloc_ast_node(parser);
    node->type = LOOP_CONTINUATION;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr += 2;

    skip_recurrent_tokens(parser, SEMI_COLON);
    return node;
}

/* Parses return expression (i.e return keyword )*/
AST_node *parse_return_expression(Parser *parser, int rec_lvl __attribute__((unused)))
{
    assert(parser);
    Token **list = parser->token_list->list;

    assert(get_keyword_type(list[parser->token_ptr]->ident) == RETURN_KEYWORD);

    AST_node *node = malloc_ast_node(parser);
    node->type = RETURN_VAL;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr++;

    enum token_type end_of_exp[] = {SEMI_COLON};
    node->ast_data.exp = parse_expression(parser, end_of_exp, 1);
    // node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);
    skip_recurrent_tokens(parser, end_of_exp[0]);
    return node;
}

/* Creates AST node and parse else (or else if) conditional block */
AST_node *parse_else_conditional(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->token_list->list;

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == ELSE_KEYWORD);

    // else if block
    if (get_keyword_type(list[parser->token_ptr + 1]->ident) == IF_KEYWORD)
    {
        AST_node *node = malloc_ast_node(parser);

        node->type = ELSE_IF_CONDITIONAL;
        node->line_num = list[parser->token_ptr]->line_num;

        // Makes sure opening parenthesis
        if (list[parser->token_ptr + 2]->type != OPEN_PARENTHESIS)
        {
            parser->token_ptr += 2;
            print_expected_token_err(parser, "Open Parenthesis ('(')", false,
                                     "Proper Syntax: else if (...) { ... } ");
            stop_parsing(parser);
            return NULL;
        }

        // parse conditional expression
        enum token_type end_of_exp[] = {CLOSING_PARENTHESIS};
        parser->token_ptr += 3;
        // node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);
        node->ast_data.exp = parse_expression(parser, end_of_exp, 1);

        // checks for open curly brackets
        if (list[parser->token_ptr]->type != OPEN_CURLY_BRACKETS)
        {
            print_expected_token_err(parser, "Open Curly Brackets ('{')", false,
                                     "Proper Syntax: else if (...) { ... }");
            stop_parsing(parser);
            return NULL;
        }
        // recursively parses inner code block
        parser->token_ptr++;
        enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
        node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

        return node;
        // regular else block
    }
    else if (list[parser->token_ptr + 1]->type == OPEN_CURLY_BRACKETS)
    {
        AST_node *node = malloc_ast_node(parser);

        node->type = ELSE_CONDITIONAL;
        node->line_num = list[parser->token_ptr]->line_num;

        enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
        parser->token_ptr += 2;
        node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

        return node;

        // syntax error
    }
    else
    {
        parser->token_ptr += 1;
        print_expected_token_err(parser, "Open Curly Brackets ('{')", false,
                                 "Proper Syntax: else { ... } ");
        stop_parsing(parser);
        return NULL;
    }

    return NULL;
}

/* Creates AST node and parses function declaration block
If its a inline function, a expression component AST node will be returned */
AST_node *parse_func_declaration(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->token_list->list;

    AccessModifier access_modifier = parse_access_modifer(parser, FUNC_KEYWORD);

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == FUNC_KEYWORD);

    // indicates inline function declaration
    if (list[parser->token_ptr + 1]->type == OPEN_PARENTHESIS)
    {

        return parse_variable_assignment_exp_func_component(parser, ++rec_lvl);
    }

    // checks for identifier
    if (list[parser->token_ptr + 1]->type != IDENTIFIER)
    {
        parser->token_ptr++;
        print_expected_token_err(parser, "Function Declaration Identifier", false,
                                 "Proper Syntax: func function ( ... ) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    // checks for open parenthesis
    if (list[parser->token_ptr + 2]->type != OPEN_PARENTHESIS)
    {
        parser->token_ptr += 2;
        print_expected_token_err(parser, "Open Parenthesis ('(')", false,
                                 "Proper Syntax: func function ( ... ) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    AST_node *node = malloc_ast_node(parser);
    node->access = access_modifier;
    node->type = FUNCTION_DECLARATION;
    node->line_num = list[parser->token_ptr]->line_num;

    node->identifier.func_name = malloc_string_cpy(parser, list[parser->token_ptr + 1]->ident);
    parser->token_ptr += 3;

    // parses function args
    ExpressionNode **args = parse_expressions_by_seperator(parser, COMMA, CLOSING_PARENTHESIS);
    node->ast_data.func_args.func_prototype_args = args;
    node->ast_data.func_args.args_num = get_pointer_list_length((void **)args);

    // checks for open parenthesis
    if (list[parser->token_ptr]->type != OPEN_CURLY_BRACKETS)
    {
        print_expected_token_err(parser, "Open Curly Brackets ('{')", false,
                                 "Proper Syntax: func function ( ... ) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    // parses function inner code block
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
    Token **list = parser->token_list->list;

    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == FUNC_KEYWORD);

    // checks for open parenthesis
    if (list[parser->token_ptr + 1]->type != OPEN_PARENTHESIS)
    {
        parser->token_ptr++;
        print_expected_token_err(parser, "Open Parenthesis ('(')", false,
                                 "Did you mean an inline function?\nProper Syntax: func ( ... ) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    AST_node *node = malloc_ast_node(parser);

    node->type = INLINE_FUNCTION_DECLARATION;
    node->line_num = list[parser->token_ptr]->line_num;
    node->identifier.func_name = NULL;

    parser->token_ptr += 2;

    // parses function args
    ExpressionNode **args = parse_expressions_by_seperator(parser, COMMA, CLOSING_PARENTHESIS);
    node->ast_data.func_args.func_prototype_args = args;
    node->ast_data.func_args.args_num = get_pointer_list_length((void **)args);

    // checks for open parenthesis
    if (list[parser->token_ptr]->type != OPEN_CURLY_BRACKETS)
    {
        print_expected_token_err(parser, "Open Curly Brackets ('{')", false,
                                 "Proper Syntax: func ( ... ) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    // parses function inner code block
    enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    parser->token_ptr++;
    node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/* Parses Object declaration */
AST_node *parse_object_declaration(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->token_list->list;

    AccessModifier access_modifier = parse_access_modifer(parser, OBJECT_KEYWORD);
    // sanity checks
    assert(get_keyword_type(list[parser->token_ptr]->ident) == OBJECT_KEYWORD);

    // checks for identifier
    if (list[parser->token_ptr + 1]->type != IDENTIFIER)
    {
        parser->token_ptr++;
        print_expected_token_err(parser, "Object Declaration Identifier", false,
                                 "Proper Syntax: object obj( ... ) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    // checks for open parenthesis
    if (list[parser->token_ptr + 2]->type != OPEN_PARENTHESIS)
    {
        parser->token_ptr += 2;
        print_expected_token_err(parser, "Open Parenthesis ('(')", false,
                                 "Proper Syntax: object obj( ... ) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    AST_node *node = malloc_ast_node(parser);
    node->access = access_modifier;
    node->type = OBJECT_DECLARATION;
    node->line_num = list[parser->token_ptr]->line_num;
    node->identifier.obj_name = malloc_string_cpy(parser, list[parser->token_ptr + 1]->ident);

    parser->token_ptr += 3;

    // parses object args
    ExpressionNode **args = parse_expressions_by_seperator(parser, COMMA, CLOSING_PARENTHESIS);
    node->ast_data.obj_args.object_prototype_args = args;
    node->ast_data.obj_args.args_num = get_pointer_list_length((void **)args);

    // checks for open parenthesis
    if (list[parser->token_ptr]->type != OPEN_CURLY_BRACKETS)
    {
        print_expected_token_err(parser, "Open Curly Brackets ('{')", false,
                                 "Proper Syntax: object obj ( ... ) { ... }");
        stop_parsing(parser);
        return NULL;
    }

    // parses object inner code block
    enum token_type end_of_block[] = {CLOSING_CURLY_BRACKETS};
    parser->token_ptr++;
    node->body = parse_code_block(parser, node, rec_lvl + 1, end_of_block, 1);

    return node;
}

/* Parses variable assignment or expression component (i.e function call) */
AST_node *parse_variable_assignment_exp_func_component(Parser *parser, int rec_lvl __attribute__((unused)))
{
    assert(parser);
    Token **list = parser->token_list->list;

    assert(is_preliminary_expression_token(list[parser->token_ptr]));

    ExpressionComponent *component = parse_expression_component(parser, NULL, 0);

    // Checks for open parenthesis
    if (list[parser->token_ptr]->type != SEMI_COLON &&
        list[parser->token_ptr]->type != ASSIGNMENT_OP)
    {
        print_expected_token_err(parser, "Semicolon (';') or Assignment Operator ('=')", false, NULL);
        stop_parsing(parser);
        return NULL;
    }

    AST_node *node = malloc_ast_node(parser);
    node->identifier.expression_component = component;
    node->line_num = list[parser->token_ptr]->line_num;

    // single expression component (func call, etc)
    if (list[parser->token_ptr]->type == SEMI_COLON)
    {
        node->type = EXPRESSION_COMPONENT;
        parser->token_ptr++;
    }
    // var assignment
    else if (list[parser->token_ptr]->type == ASSIGNMENT_OP)
    {
        node->type = VAR_ASSIGNMENT;
        parser->token_ptr++;
        enum token_type end_of_exp[] = {SEMI_COLON};
        // node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);
        node->ast_data.exp = parse_expression(parser, end_of_exp, 1);
    }

    skip_recurrent_tokens(parser, SEMI_COLON);

    return node;
}

/* Recursively parses code block and returns a ast_node (abstract syntax tree node)*/
/* Function will always terminate with the parser->token_ptr pointing on the end exp token + 1*/
AST_List *parse_code_block(
    Parser *parser,
    AST_node *parent_block,
    int rec_lvl,
    enum token_type ends_of_block[],
    const int ends_of_block_length)
{

    assert(parser);
    Token **list = parser->token_list->list;

    if (is_lexeme_in_list(list[parser->token_ptr]->type, ends_of_block, ends_of_block_length))
    {
        parser->token_ptr++;
        return NULL;
    }

    // SYNTAX ERROR: invalid end of code block
    if (list[parser->token_ptr]->type == END_OF_FILE)
    {
        print_unexpected_end_of_file_err(parser, NULL);
        stop_parsing(parser);
        return NULL;
    }

    AST_List *ast_list = malloc_ast_list(parser);

    // loops until it encounters a end of block token
    while (!is_lexeme_in_list(list[parser->token_ptr]->type, ends_of_block, ends_of_block_length))
    {
        if (parser->error_indicator)
        {
            break;
        }

        switch (get_keyword_type(list[parser->token_ptr]->ident))
        {

        // variable declaration
        case LET_KEYWORD:
        {
        let_keyword:; // label

            AST_node *node = parse_variable_declaration(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }
        // WHILE loop
        case WHILE_KEYWORD:
        {
            AST_node *node = parse_while_loop(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // if conditional
        case IF_KEYWORD:
        {
            AST_node *node = parse_if_conditional(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // else conditional
        case ELSE_KEYWORD:
        {
            AST_node *node = parse_else_conditional(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // break (loop termination)
        case BREAK_KEYWORD:
        {
            AST_node *node = parse_loop_termination(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // continue (loop continuation)
        case CONTINUE_KEYWORD:
        {
            AST_node *node = parse_loop_continuation(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // return
        case RETURN_KEYWORD:
        {

            AST_node *node = parse_return_expression(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // function declaration
        case FUNC_KEYWORD:
        {
        func_keyword:; // label

            AST_node *node = parse_func_declaration(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // TODO
        case OBJECT_KEYWORD:
        {
        object_keyword:;

            AST_node *node = parse_object_declaration(parser, rec_lvl);
            push_to_ast_list(ast_list, node);
            break;
        }

        // handles access modifiers
        case GLOBAL_KEYWORD:
        case PRIVATE_KEYWORD:
        {
            if (list[parser->token_ptr + 1]->type == KEYWORD)
            {
                switch (get_keyword_type(list[parser->token_ptr + 1]->ident))
                {
                case FUNC_KEYWORD:
                    goto func_keyword;
                case LET_KEYWORD:
                    goto let_keyword;
                case OBJECT_KEYWORD:
                    goto object_keyword;
                default:
                    break;
                }
            }

            parser->error_indicator = true;

            if (ast_list->head && ast_list->head->type == FUNCTION_DECLARATION &&
                list[parser->token_ptr]->type == OPEN_PARENTHESIS)
            {

                print_invalid_access_modifer_err(parser, list[parser->token_ptr]->ident,
                                                 "Access Modifer keywords can only be used in front of variable, function or object declarations"
                                                 "\nDid you mean an inline function?"
                                                 "\nProper Syntax: func ( ... ) {...}(ARGS); ");
            }
            else
            {

                print_invalid_access_modifer_err(parser, list[parser->token_ptr]->ident,
                                                 "Access Modifer keywords can only be used in front of variable, function or object declarations");
            }

            stop_parsing(parser);
            break;
        }

        default:
        {
            // TODO: Seperate this logic into a seperate function
            // parses variable assignment
            // Grammar: [EXPRESSION COMPONENT] [ASSIGNMENT OP] [EXPRESSION][END OF EXPRESSION]
            // OR parses function calls
            // Grammar [EXPRESSION COMPONENT] -> [IDENTIFER][ARGUMENTS ...][END OF EXPRESSION]
            if (is_preliminary_expression_token(list[parser->token_ptr]))
            {
                AST_node *node = parse_variable_assignment_exp_func_component(parser, rec_lvl);
                push_to_ast_list(ast_list, node);
            }
            else
            {

                if (ast_list->tail &&
                    ast_list->tail->type == FUNCTION_DECLARATION &&
                    list[parser->token_ptr]->type == OPEN_PARENTHESIS)
                {

                    print_invalid_token_err(parser,
                                            "Did you mean an inline function?"
                                            "\nProper Syntax: func ( ... ) { ... }(ARGUMENTS); ");
                }
                else if (list[parser->token_ptr]->type == END_OF_FILE)
                {
                    print_unexpected_end_of_file_err(parser, NULL);
                }
                else
                {
                    print_invalid_token_err(parser, NULL);
                }

                stop_parsing(parser);
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
