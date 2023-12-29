#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "errors.h"
#include "keywords.h"
#include "parser.h"
#include "semanalysis.h"

// ANSI escape codes for text color
#define RED_TEXT "\x1b[31m"
#define GREEN_TEXT "\x1b[32m"
#define YELLOW_TEXT "\x1b[33m"
#define BLUE_TEXT "\x1b[34m"
#define MAGENTA_TEXT "\x1b[35m"
#define CYAN_TEXT "\x1b[36m"
#define WHITE_TEXT "\x1b[37m"
#define RESET_COLOR "\x1b[0m"

/* Prints out syntax message depending on parsing context */
/* Only used for parsing errors (not semantic analyzer)*/
static void print_ctx_dependent_msg(ParsingContext ctx, char *color)
{
    switch (ctx)
    {
    case REGUALR_CTX:
        return;

    case LIST_CTX:
    {
        printf("%s"
               "Proper List Syntax: [1,2,3,4,5,6,7, ...];\n" RESET_COLOR,
               color);
        return;
    }
    case MAP_CTX:
    {
        printf("%s"
               "Proper Map Syntax: map {key1: val1, key2: val2, .... };\n" RESET_COLOR,
               color);
        return;
    }
    case SET_CTX:
    {
        printf("%s"
               "Proper Set Syntax: set {val1, val2, val3, ...};\n" RESET_COLOR,
               color);
        return;
    }
    }
}

/* Converts a expression component into a string */
static char *_exp_component_to_string(ExpressionComponent *node)
{
    assert(node);
    switch (node->type)
    {
    case NUMERIC_CONSTANT:
        return "Numeric Constant";
    case STRING_CONSTANT:
        return "String Constant";
    case LIST_CONSTANT:
        return "List Constant";
    case NULL_CONSTANT:
        return "NULL Constant";
    case VARIABLE:
        return "Variable Identifier";
    case LIST_INDEX:
        return "Index Expression";
    case FUNC_CALL:
        return "Function Call";
    case INLINE_FUNC:
        return "Inline Function";
    default:
        return NULL;
    }
}

static void _print_whitespace(unsigned num)
{
    for (int i = 0; i < (int)num; i++)
        printf(" ");
}

#define DEFAULT_POINTER_LENGTH 6
#define OFFSET_ADDON 8
static void _print_pointer(unsigned offset)
{
    assert(offset >= 0);

    _print_whitespace(offset + OFFSET_ADDON);
    for (int i = 0; i < DEFAULT_POINTER_LENGTH; i++)
    {
        printf(YELLOW_TEXT "%c", '^');
    }
    printf("\n" RESET_COLOR);
}

static void _print_prev_lines(char **lines, unsigned cur_line, unsigned top_line)
{
    int i = cur_line - top_line - 1;
    for (; i < (int)cur_line; i++)
    {
        if (i < 0)
            continue;
        printf(BLUE_TEXT "%d" CYAN_TEXT " |" RESET_COLOR, i + 1);
        if (i == (int)cur_line - 1)
        {
            printf(RED_TEXT "      %s\n" RESET_COLOR, lines[i]);
            continue;
        }
        printf(GREEN_TEXT "      %s\n" RESET_COLOR, lines[i]);
    }
}

/* Prints the code */
static void _print_context(Token **list, const int token_ptr, const char *filename, char **lines)
{
    int line_nb = list[token_ptr]->line_num;
    int line_pos = list[token_ptr]->line_pos;

    printf(RED_TEXT "--- " MAGENTA_TEXT "[%s]" RED_TEXT " Error at [%d:%d] ---\n" RESET_COLOR, filename, line_nb, line_pos);

    if (token_ptr > 0 && list[token_ptr - 1]->line_num + 4 < line_nb)
    {
        _print_prev_lines(lines, list[token_ptr - 1]->line_num, 3);
        printf(BLUE_TEXT "." BLUE_TEXT);
        _print_pointer(list[token_ptr - 1]->line_pos);
        printf(BLUE_TEXT ".\n" BLUE_TEXT);
    }

    _print_prev_lines(lines, line_nb, 4);
    _print_pointer(line_pos);
}

/* Print Error missing operator in Expression */
void print_missing_operator_err(Parser *parser, const char *msg)
{
    Token **list = parser->token_list->list;
    _print_context(list, parser->token_ptr, parser->file_name, parser->lines.lines);

    if (list[parser->token_ptr]->type == KEYWORD)
    {
        printf(RED_TEXT "Expected Binary Operator but got reserved keyword '%s'\n" RESET_COLOR, list[parser->token_ptr]->ident);
    }
    else
    {
        printf(RED_TEXT "Expected Binary Operator but got '%s'\n" RESET_COLOR, list[parser->token_ptr]->ident);
    }

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);

    print_ctx_dependent_msg(parser->ctx, RED_TEXT);
}

/* Print Error missing operator in Expression */
void print_missing_exp_component_err(Parser *parser, const char *msg)
{
    Token **list = parser->token_list->list;

    _print_context(list, parser->token_ptr, parser->file_name, parser->lines.lines);

    printf(RED_TEXT "Expected Expression Component but got %s '%s'\n" RESET_COLOR,
           list[parser->token_ptr]->type == KEYWORD ? "reserved keyword" : "",
           list[parser->token_ptr]->ident);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);

    print_ctx_dependent_msg(parser->ctx, RED_TEXT);
}

/* Print Error missing operator in Expression */
void print_invalid_token_err(Parser *parser, const char *msg)
{
    Token **list = parser->token_list->list;

    _print_context(list, parser->token_ptr, parser->file_name, parser->lines.lines);
    printf(RED_TEXT "Invalid Token '%s'.\n" RESET_COLOR,
           list[parser->token_ptr]->ident);

    if (list[parser->token_ptr]->type == KEYWORD)
        printf(RED_TEXT "'%s' is a reserved keyword\n" RESET_COLOR, list[parser->token_ptr]->ident);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);

    print_ctx_dependent_msg(parser->ctx, RED_TEXT);
}

/* Print Error expected token  */
void print_expected_token_err(Parser *parser, const char *expected_token, const bool is_keyword, const char *msg)
{
    Token **list = parser->token_list->list;

    if (list[parser->token_ptr]->type == END_OF_FILE)
    {
        print_unexpected_end_of_file_err(parser, msg);
        return;
    }

    _print_context(list, parser->token_ptr, parser->file_name, parser->lines.lines);

    printf(RED_TEXT "Expected %s %s, but got %s '%s'\n" RESET_COLOR,
           expected_token,
           is_keyword ? "Keyword" : "Token",
           list[parser->token_ptr]->type == KEYWORD ? "reserved keyword" : "",
           list[parser->token_ptr]->ident);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);

    print_ctx_dependent_msg(parser->ctx, RED_TEXT);
}

/* Prints Error for invalid access modifier error */
void print_invalid_access_modifer_err(Parser *parser, const char *keyword, const char *msg)
{
    Token **list = parser->token_list->list;

    _print_context(list, parser->token_ptr, parser->file_name, parser->lines.lines);

    if (parser->token_ptr < (int)parser->token_list->len)
    {
        char *next_identifier = list[parser->token_ptr + 1]->ident;

        printf(RED_TEXT "Invalid use of '%s' Access Modifier, '%s' must be followed by a variable, function, or Object declaration, but got %s '%s'\n" RESET_COLOR,
               keyword,
               keyword,
               list[parser->token_ptr + 1]->type == KEYWORD ? "reserved keyword" : "token",
               next_identifier);
    }
    else
    {
        printf(RED_TEXT "Invalid use of '%s' Access Modifier, '%s' must be followed by a variable, function, or Object declaration" RESET_COLOR, keyword, keyword);
    }

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);

    print_ctx_dependent_msg(parser->ctx, RED_TEXT);
}

/* Prints out unexpected end of file error */
void print_unexpected_end_of_file_err(Parser *parser, const char *msg)
{
    _print_context(parser->token_list->list, parser->token_ptr, parser->file_name, parser->lines.lines);

    printf(RED_TEXT "Encountered unexpected end of file\n" RESET_COLOR);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);

    print_ctx_dependent_msg(parser->ctx, RED_TEXT);
}

/* Prints out invalid expression component */
void print_invalid_expression_component(Parser *parser, const char *msg)
{
    _print_context(parser->token_list->list, parser->token_ptr, parser->file_name, parser->lines.lines);

    printf(RED_TEXT "Encountered Invalid expression component\n" RESET_COLOR);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);

    print_ctx_dependent_msg(parser->ctx, RED_TEXT);
}

/* SEMANTIC ERRORS */

void print_undeclared_identifier_err(SemanticAnalyzer *sa, ExpressionComponent *comp, const char *msg)
{
    assert(comp->type == VARIABLE);

    _print_context(sa->token_list->list, comp->token_num, sa->filename, sa->lines.lines);
    printf(
        RED_TEXT
        "Identifier '%s' is not defined\n"
        "Define the variable '%s': let %s = ...;\n" RESET_COLOR,
        comp->meta_data.variable_reference,
        comp->meta_data.variable_reference,
        comp->meta_data.variable_reference);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints invalid argument identifer error */
void print_invalid_arg_identifier_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);
    printf(RED_TEXT "Invalid argument '%s' expression\n" RESET_COLOR, sa->token_list->list[token_ptr]->ident);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out invalid access modifier error */
void print_invalid_access_modifier_semantics_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);
    printf(RED_TEXT "Invalid access modifier for '%s' \n" RESET_COLOR, sa->token_list->list[token_ptr]->ident);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out invalid access modifier error */
void print_invalid_object_block_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);
    printf(RED_TEXT "'%s' is not a valid code block in the Object scope.\n" RESET_COLOR, sa->token_list->list[token_ptr]->ident);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints invalid sub component error (i.e [EXPRESSION COMPONENT] -> [TERMINAL COMPONENT]) */
void print_invalid_terminal_top_component_err(SemanticAnalyzer *sa, ExpressionComponent *cm, const char *msg)
{
    _print_context(sa->token_list->list, cm->token_num, sa->filename, sa->lines.lines);

    assert(cm->top_component);

    printf("%s cannot be a child of %s",
           _exp_component_to_string(cm),
           _exp_component_to_string(cm->sub_component));

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out invalid function call */
void print_invalid_func_call_err(SemanticAnalyzer *sa, ExpressionComponent *cm, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    printf(RED_TEXT "Cannot make function call on %s.\n" RESET_COLOR,
           _exp_component_to_string(cm->sub_component));

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out invalid index */
void print_invalid_index_err(SemanticAnalyzer *sa, ExpressionComponent *cm, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    printf(RED_TEXT "Cannot take index of %s.\n" RESET_COLOR,
           _exp_component_to_string(cm->sub_component));

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out else if error  */
void print_invalid_else_if_block_err(SemanticAnalyzer *sa, AST_node *node, const int token_ptr, const char *msg)
{
    assert(node->type == ELSE_IF_CONDITIONAL);
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    printf(RED_TEXT
           "Invalid ELSE IF block. "
           "ELSE IF blocks must preceded by IF or ELSE IF blocks.\n" RESET_COLOR);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out else if error  */
void print_empty_exp_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    printf(RED_TEXT
           "Expression cannot be empty.\n" RESET_COLOR);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out else if error  */
void print_invalid_else_block_err(SemanticAnalyzer *sa, AST_node *node, const int token_ptr, const char *msg)
{
    assert(node->type == ELSE_CONDITIONAL);
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    printf(RED_TEXT
           "Invalid ELSE block. "
           "ELSE blocks must preceded by IF or ELSE IF blocks.\n" RESET_COLOR);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out invalid ast node  */
void print_invalid_ast_node(SemanticAnalyzer *sa, AST_node *node, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    printf(RED_TEXT
           "Invalid statement\n"
            RESET_COLOR);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out else if error  */
void print_invalid_var_assignment_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    printf(RED_TEXT
           "Invalid Variable Assignment. %s\n" RESET_COLOR,
           msg);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out invalid empty body error */
void print_invalid_empty_body_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    printf(RED_TEXT
           "Empty body is invalid. \n" RESET_COLOR);

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out invalid number of arguments error */
void print_invalid_arg_count_err(
    SemanticAnalyzer *sa,
    const int arg_count, const int expected_arg_count,
    const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    if (expected_arg_count == 1)
    {
        printf(RED_TEXT "Function expected 1 Argument, but got %d \n" RESET_COLOR,
               arg_count);
    }
    else
    {
        printf(RED_TEXT "Function expected %d Arguments, but got %d \n" RESET_COLOR,
               expected_arg_count, arg_count);
    }

    if (msg)
        printf(RED_TEXT "%s\n" RESET_COLOR, msg);
}

/* Prints out invalid global return value */
void print_invalid_global_return_value(SemanticAnalyzer *sa, const int token_ptr, const char *msg)
{
    _print_context(sa->token_list->list, token_ptr, sa->filename, sa->lines.lines);

    printf(RED_TEXT
           "%s \n" RESET_COLOR, msg);

}