#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <setjmp.h>
#include <math.h>
#include "keywords.h"
#include "parser.h"
#include "errors.h"

/* Makes a long jump and returns to when the parsing function was called */
static void stop_parsing(Parser *parser)
{
    parser->error_indicator = true;
    longjmp(*parser->error_handler, 1);
}

/* Parses access modifer, ["private" | "global"] [KEYWORD] */
static AccessModifier parse_access_modifer(Parser *parser, enum keyword_type next_keyword)
{
    assert(parser);

    Token **list = parser->lexeme_list->list;
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

/* Skips all recurrent tokens */
static void skip_recurrent_tokens(Parser *parser, enum token_type type)
{
    Token **list = parser->lexeme_list->list;
    while (list[parser->token_ptr]->type == type)
        parser->token_ptr++;
}

/* Mallocs Parser struct */
Parser *malloc_parser()
{
    Parser *parser = malloc(sizeof(Parser));
    parser->error_indicator = false;
    parser->lexeme_list = NULL;
    parser->memtracker = init_memtracker();
    parser->token_ptr = 0;
    parser->file_name = NULL;
    return parser;
}

/* Frees parser */
void free_parser(Parser *parser)
{
    if (!parser)
        return;

    free_token_list(parser->lexeme_list);
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
    Token **list = parser->lexeme_list->list;
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
char *malloc_string_cpy(Parser *parser, const char *str)
{
    char *str_cpy = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(str_cpy, str);

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

    if (parser)
        push_to_memtracker(parser->memtracker, node, free);

    return node;
}

/* Checks wether lexeme_type is a valid beginning of expression component,
Is also valid for func keyword (inline functions syntax)
*/
bool is_preliminary_expression_token(Token *lexeme)
{
    return lexeme->type == IDENTIFIER ||
           lexeme->type == OPEN_SQUARE_BRACKETS ||
           lexeme->type == NUMERIC_LITERAL ||
           lexeme->type == STRING_LITERALS ||
           (lexeme->type == KEYWORD && (get_keyword_type(lexeme->ident) == FUNC_KEYWORD ||
                                        get_keyword_type(lexeme->ident) == NULL_KEYWORD));
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

#define DEFAULT_ARG_LIST_LENGTH 1

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
    while (parser->lexeme_list->list[parser->token_ptr - 1]->type != end_of_exp)
    {
        ExpressionNode *tmp = parse_expression(parser, NULL, NULL, seperators, 2);

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

/* Gets the length of the argument list (ends with a NULL pointer)*/
int get_expression_list_length(ExpressionNode **args)
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

Note: This function does not check for a end of expression token, it knows when to end the recursion
*/
ExpressionComponent *parse_expression_component(
    Parser *parser,
    ExpressionComponent *parent,
    int rec_lvl)
{
    assert(parser);
    assert(rec_lvl >= 0);

    Token **list = parser->lexeme_list->list;

    // Base cases
    if (list[parser->token_ptr]->type == END_OF_FILE)
    {
        print_unexpected_end_of_file_err(parser, NULL);
        stop_parsing(parser);
        return parent;
    }

    if (parent)
    {

        if ((
                parent->type == FUNC_CALL ||
                parent->type == LIST_INDEX ||
                parent->type == NUMERIC_CONSTANT ||
                parent->type == STRING_CONSTANT ||
                parent->type == LIST_CONSTANT ||
                parent->type == VARIABLE ||
                parent->type == INLINE_FUNC ||
                parent->type == NULL_CONSTANT) &&
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

    ExpressionComponent *component = malloc_expression_component(parser);
    component->line_num = list[parser->token_ptr]->line_num;

    // void pointer (null pointer)
    if (list[parser->token_ptr]->type == KEYWORD && get_keyword_type(list[parser->token_ptr]->ident) == NULL_KEYWORD)
    {
        component->type = NULL_CONSTANT;
        parser->token_ptr++;
    }

    // Parse list constants (Cannot have parent component)
    else if (rec_lvl == 0 && !parent && list[parser->token_ptr]->type == OPEN_SQUARE_BRACKETS)
    {
        component->type = LIST_CONSTANT;
        parser->token_ptr++;
        ExpressionNode **elements = parse_expressions_by_seperator(parser, COMMA, CLOSING_SQUARE_BRACKETS);
        component->meta_data.list_const.list_elements = elements;
        component->meta_data.list_const.list_length = get_expression_list_length(elements);
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
        component->meta_data.list_index = parse_expression(parser, NULL, NULL, end_of_exp, 1);

        // function call
    }
    else if (list[parser->token_ptr]->type == OPEN_PARENTHESIS)
    {
        // SYNTAX ERROR: INVALID USE OF ARROW TOKEN
        if (list[parser->token_ptr - 1]->type == ATTRIBUTE_ARROW)
        {
            parser->error_indicator = true;
        }

        component->type = FUNC_CALL;
        parser->token_ptr++;
        ExpressionNode **args = parse_expressions_by_seperator(parser, COMMA, CLOSING_PARENTHESIS);
        component->meta_data.func_data.func_args = args;
        component->meta_data.func_data.args_num = get_expression_list_length(args);
    }
    else
    {   
        // gives a tailored error messages
        switch(list[parser->token_ptr]->type) {
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

// uses reverse polish notation
ExpressionNode *parse_expression(
    Parser *parser,
    ExpressionNode *LHS,
    ExpressionNode *RHS,
    enum token_type ends_of_exp[],
    const int ends_of_exp_length)
{
    assert(parser);

    Token **list = parser->lexeme_list->list;

    if (list[parser->token_ptr]->type == END_OF_FILE)
    {
        print_unexpected_end_of_file_err(parser, NULL);
        stop_parsing(parser);
        return LHS;
    }

    // Base case (meet end of expression token)
    if (is_lexeme_in_list(list[parser->token_ptr]->type, ends_of_exp, ends_of_exp_length))
    {
        // ONLY LHS should be non-NULL when expression ends
        // If not, an operator is missing
        if (LHS && RHS)
        {
            parser->error_indicator = true;
            print_missing_operator_err(parser, NULL);
            stop_parsing(parser);
        }

        parser->token_ptr++;
        return LHS;
    }

    bool is_exp_negated = false;

    // handles negation operator (i.e '!')
    while (list[parser->token_ptr]->type == LOGICAL_NOT_OP)
    {
        is_exp_negated = !is_exp_negated;
        parser->token_ptr++;
    }

    // Handles and Computes sub expressions
    if (list[parser->token_ptr]->type == OPEN_PARENTHESIS)
    {
        parser->token_ptr++;
        enum token_type end_of_exp[] = {CLOSING_PARENTHESIS};
        ExpressionNode *sub_exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);
        sub_exp->negation = is_exp_negated ^ sub_exp->negation;

        if (!LHS)
            return parse_expression(parser, sub_exp, RHS, ends_of_exp, ends_of_exp_length);
        else
            return parse_expression(parser, LHS, sub_exp, ends_of_exp, ends_of_exp_length);
    }

    ExpressionNode *node = malloc_expression_node(parser);
    node->negation = is_exp_negated;
    // checks if we encountered a expression component
    if (is_preliminary_expression_token(list[parser->token_ptr]))
    {

        node->component = parse_expression_component(parser, NULL, 0);
        node->type = VALUE;

        if (!LHS)
            return parse_expression(parser, node, RHS, ends_of_exp, ends_of_exp_length);
        else
            return parse_expression(parser, LHS, node, ends_of_exp, ends_of_exp_length);
    }

    // operator must have two sub trees
    if (!LHS || !RHS)
    {
        parser->error_indicator = true;
        print_missing_exp_component_err(parser,
                                        "Make sure to have two expression components before applying operator");
        stop_parsing(parser);
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

    case LOGICAL_NOT_OP:
    default:
    {
        print_missing_operator_err(parser, NULL);
        parser->error_indicator = true;
        return LHS;
    }
    }

    node->LHS = LHS;
    node->RHS = RHS;

    parser->token_ptr++;

    return parse_expression(parser, node, NULL, ends_of_exp, ends_of_exp_length);
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

void push_to_ast_list(volatile AST_List *list, AST_node *node)
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
    Token **list = parser->lexeme_list->list;

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

    // checks for assignment operator
    if (list[parser->token_ptr + 2]->type != ASSIGNMENT_OP)
    {
        parser->token_ptr += 2;
        print_expected_token_err(parser, "Assignment Operator ('=')", false,
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

    // parses its assignment value (i.e an expression)
    parser->token_ptr += 3;
    enum token_type end_of_exp[] = {SEMI_COLON};
    node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

    skip_recurrent_tokens(parser, end_of_exp[0]);
    return node;
}

/* Creates AST node and parses while loop block */
AST_node *parse_while_loop(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->lexeme_list->list;

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
    node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

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
    Token **list = parser->lexeme_list->list;

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
    node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

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
AST_node *parse_loop_termination(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->lexeme_list->list;

    assert(get_keyword_type(list[parser->token_ptr]->ident) == BREAK_KEYWORD);

    // checks syntax
    if(list[parser->token_ptr+1]->type != SEMI_COLON) {
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
AST_node *parse_loop_continuation(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->lexeme_list->list;

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
AST_node *parse_return_expression(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->lexeme_list->list;

    assert(get_keyword_type(list[parser->token_ptr]->ident) == RETURN_KEYWORD);

    AST_node *node = malloc_ast_node(parser);
    node->type = RETURN_VAL;
    node->line_num = list[parser->token_ptr]->line_num;

    parser->token_ptr++;

    enum token_type end_of_exp[] = {SEMI_COLON};
    node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);
    skip_recurrent_tokens(parser, end_of_exp[0]);
    return node;
}

/* Creates AST node and parse else (or else if) conditional block */
AST_node *parse_else_conditional(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->lexeme_list->list;

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
        node->ast_data.exp = parse_expression(parser, NULL, NULL, end_of_exp, 1);

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
    } else {
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
    Token **list = parser->lexeme_list->list;

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
    node->ast_data.func_args.args_num = get_expression_list_length(args);

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
    Token **list = parser->lexeme_list->list;

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
    node->ast_data.func_args.args_num = get_expression_list_length(args);

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
    Token **list = parser->lexeme_list->list;

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
    node->ast_data.obj_args.args_num = get_expression_list_length(args);

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
AST_node *parse_variable_assignment_exp_func_component(Parser *parser, int rec_lvl)
{
    assert(parser);
    Token **list = parser->lexeme_list->list;

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
    Token **list = parser->lexeme_list->list;

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

            if (ast_list->head->type == FUNCTION_DECLARATION &&
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

                if (ast_list->tail->type == FUNCTION_DECLARATION &&
                    list[parser->token_ptr]->type == OPEN_PARENTHESIS)
                {

                    print_invalid_token_err(parser,
                                            "Did you mean an inline function?"
                                            "\nProper Syntax: func ( ... ) { ... }(ARGUMENTS); ");
                }
                else if(list[parser->token_ptr]->type == END_OF_FILE) 
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
