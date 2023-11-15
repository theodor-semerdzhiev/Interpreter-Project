#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "errors.h"
#include "keywords.h"
#include "parser.h"

// ANSI escape codes for text color
#define RED_TEXT "\x1b[31m"
#define GREEN_TEXT "\x1b[32m"
#define YELLOW_TEXT "\x1b[33m"
#define BLUE_TEXT "\x1b[34m"
#define MAGENTA_TEXT "\x1b[35m"
#define CYAN_TEXT "\x1b[36m"
#define WHITE_TEXT "\x1b[37m"
#define RESET_COLOR "\x1b[0m"

static char* _parser_getNthLine(Parser *parser , unsigned line) {
    return parser->lines.lines[line-1];
}

static void _print_whitespace(unsigned num) {
    for(int i=0; i < num; i++)
        printf(" ");
}


#define DEFAULT_POINTER_LENGTH 6
#define OFFSET_ADDON 8
static void _print_pointer(unsigned offset) {
    assert(offset >= 0);

    _print_whitespace(offset+OFFSET_ADDON);
    for(int i=0; i < DEFAULT_POINTER_LENGTH; i++) {
        printf(YELLOW_TEXT "%c", '^');
    }
    printf("\n" RESET_COLOR);
}

static void _print_prev_lines(char** lines, unsigned cur_line, unsigned top_line) {
    assert(cur_line > top_line);

    for(int i=cur_line-top_line-1; i < cur_line; i++) {
        if(i < 0) continue;
        printf(BLUE_TEXT "%d" CYAN_TEXT " |" RESET_COLOR, i+1);
        if(i == cur_line-1) {
            printf(RED_TEXT "      %s\n" RESET_COLOR,lines[i]);
            continue;
        }
        printf(GREEN_TEXT "      %s\n" RESET_COLOR, lines[i]);
    }
}

/* Prints the code */
static void _print_context(Parser *parser) {
    Token **list = parser->lexeme_list->list;

    int line_nb = list[parser->token_ptr]->line_num;
    int line_pos = list[parser->token_ptr]->line_pos;

    printf(RED_TEXT "--- " MAGENTA_TEXT "[%s]" RED_TEXT " Error at [%d:%d] ---\n" RESET_COLOR, parser->file_name ,line_nb, line_pos);

    if(parser->token_ptr > 0 && list[parser->token_ptr-1]->line_num+4 < line_nb) {
        _print_prev_lines(parser->lines.lines, list[parser->token_ptr-1]->line_num, 3);
        printf(BLUE_TEXT "." BLUE_TEXT);
        _print_pointer(list[parser->token_ptr-1]->line_pos);
        printf(BLUE_TEXT ".\n" BLUE_TEXT);

    }

    _print_prev_lines(parser->lines.lines, line_nb, 4);
    _print_pointer(line_pos);
}

/* Print Error missing operator in Expression */
void print_missing_operator_err(Parser *parser) {
    _print_context(parser);
    Token **list = parser->lexeme_list->list;
    printf(RED_TEXT "Expected Binary Operator but got '%s'\n" RESET_COLOR, list[parser->token_ptr]->ident);
}

/* Print Error missing operator in Expression */
void print_missing_exp_component_err(Parser *parser) {
    _print_context(parser);
    Token **list = parser->lexeme_list->list;
    printf(RED_TEXT "Expected Expression Component but got '%s'\n" RESET_COLOR, list[parser->token_ptr]->ident);
}

/* Print Error missing operator in Expression */
void print_invalid_token_err(Parser *parser) {
    _print_context(parser);
    Token **list = parser->lexeme_list->list;
    printf(RED_TEXT "Invalid Token %s\n" RESET_COLOR, list[parser->token_ptr]->ident);
}

/* Print Error expected token  */
void print_expected_token_err(Parser *parser, const char* expected_token, const bool is_keyword) {
    _print_context(parser);

    Token **list = parser->lexeme_list->list;

    if(is_keyword) {
        printf(RED_TEXT "Expected %s Keyword, but got '%s'\n" RESET_COLOR, 
        expected_token, 
        list[parser->token_ptr]->ident);
    } else {
        printf(RED_TEXT "Expected %s Token, but got '%s'\n" RESET_COLOR, 
        expected_token, 
        list[parser->token_ptr]->ident);
    }
    
}

/* Prints Error for invalid access modifier error */
void print_invalid_access_modifer_err(Parser *parser, const char* keyword) {
    _print_context(parser);

    Token **list = parser->lexeme_list->list;

    if(parser->token_ptr < parser->lexeme_list->len) {
        char* next_identifier = list[parser->token_ptr+1]->ident;

        printf(RED_TEXT "Invalid use of '%s' Access Modifier, '%s' must be followed by a variable, function, or Object declaration, but got token '%s'\n" RESET_COLOR, keyword, keyword, next_identifier);
    } else {

        printf(RED_TEXT "Invalid use of '%s' Access Modifier, '%s' must be followed by a variable, function, or Object declaration" RESET_COLOR, keyword, keyword);
    }
}
