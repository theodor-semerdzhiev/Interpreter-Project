#pragma once
#include "../parser/parser.h"

void print_token_list(TokenList *lexemes);

void print_expression_component(ExpressionComponent *component, char *buffer, int rec_lvl);

void print_expression_tree(ExpressionNode *root, char *buffer, int rec_lvl);
void print_ast_node(AST_node *node, char *buffer, int rec_lvl);
void print_ast_list(AST_List *list, char *buffer, int rec_lvl);
