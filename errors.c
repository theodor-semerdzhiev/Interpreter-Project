#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "errors.h"
#include "keywords.h"
#include "parser.h"

static char* _getNthLine(Parser *parser , unsigned line) {
    return parser->lines.lines[line-1];
}

static void _print_whitespace(unsigned num) {

    for(int i=0; i < num; i++)
        printf(" ");
}

#define POINTER_WIDTH 6
static void _print_pointer(unsigned offset) {
    assert(offset >= 0);

    _print_whitespace(offset+5);
    for(int i=0; i < POINTER_WIDTH; i++) {
        printf("%c", '^');
    }
    printf("\n");
}

static void _print_prev_lines(char** lines, unsigned cur_line, unsigned top_line) {
    assert(cur_line > top_line);

    for(int i=cur_line-top_line; i < cur_line; i++) {
        if(i < 0) continue;
        printf("%d      %s\n", i+1, lines[i]);
    }
}

/* Print Error missing operator in Expression */
void print_missing_operator_err(Parser *parser) {
    Token **list = parser->lexeme_list->list;

    int line_nb = list[parser->token_ptr]->line_num;
    int line_pos = list[parser->token_ptr]->line_pos;

    printf("[%s] Error at [%d:%d]\n", parser->file_name ,line_nb, line_pos);
    _print_prev_lines(parser->lines.lines, line_nb, 4);
    _print_pointer(line_pos);
    printf("Expected Binary Operator but got '%s'\n", list[parser->token_ptr]->ident);
}

/* Print Error missing operator in Expression */
void print_missing_exp_component_err(Parser *parser) {
    Token **list = parser->lexeme_list->list;

    int line_nb = list[parser->token_ptr]->line_num;
    int line_pos = list[parser->token_ptr]->line_pos;

    printf("[%s] Error at [%d:%d]\n", parser->file_name, line_nb, line_pos);
    _print_prev_lines(parser->lines.lines, line_nb, 4);
    _print_pointer(line_pos);
    printf("Expected Expression Component but got '%s'\n", list[parser->token_ptr]->ident);
}

/* Print Error missing operator in Expression */
void print_invalid_token_err(Parser *parser) {
    Token **list = parser->lexeme_list->list;

    int line_nb = list[parser->token_ptr]->line_num;
    int line_pos = list[parser->token_ptr]->line_pos;

    printf("[%s] Error at [%d:%d]\n", parser->file_name, line_nb, line_pos);
    _print_prev_lines(parser->lines.lines, line_nb, 4);
    _print_pointer(line_pos);
    printf("Invalid Token %s\n", list[parser->token_ptr]->ident);
}

/* Print Error expected token  */
void print_expected_token_err(Parser *parser, const char* expected_token, const bool is_keyword) {
    Token **list = parser->lexeme_list->list;

    int line_nb = list[parser->token_ptr]->line_num;
    int line_pos = list[parser->token_ptr]->line_pos;

    printf("[%s] Error at [%d:%d]\n", parser->file_name, line_nb, line_pos);
    _print_prev_lines(parser->lines.lines, line_nb, 4);
    _print_pointer(line_pos);

    if(is_keyword) {
        printf("Expected %s Keyword, but got '%s'\n", 
        expected_token, 
        list[parser->token_ptr]->ident);
    } else {
        printf("Expected %s Token, but got '%s'\n", 
        expected_token, 
        list[parser->token_ptr]->ident);
    }
    
}
