#include "parser.h"
#include "semanalysis.h"

#ifndef ERRORS
#define ERRORS

void print_missing_operator_err(Parser *parser, const char *msg);
void print_missing_exp_component_err(Parser *parser, const char *msg);
void print_invalid_token_err(Parser *parser, const char *msg);
void print_expected_token_err(Parser *parser, const char *expected_token, const bool is_keyword, const char *msg);
void print_invalid_access_modifer_err(Parser *parser, const char *keyword, const char *msg);
void print_unexpected_end_of_file_err(Parser *parser, const char *msg);
void print_invalid_expression_component(Parser *parser, const char *msg);

/* Semantic errors */
void print_undeclared_identifier_err(SemanticAnalyzer *sa, ExpressionComponent *comp, const char *msg);
void print_invalid_arg_identifier_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_access_modifier_semantics_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_object_block_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_terminal_top_component_err(SemanticAnalyzer *sa, ExpressionComponent *cm, const char *msg);
void print_invalid_func_call_err(SemanticAnalyzer *sa, ExpressionComponent *cm, const int token_ptr, const char *msg);
void print_invalid_index_err(SemanticAnalyzer *sa, ExpressionComponent *cm, const int token_ptr, const char *msg);
void print_invalid_else_if_block_err(SemanticAnalyzer *sa, AST_node *node, const int token_ptr, const char *msg);
void print_invalid_else_block_err(SemanticAnalyzer *sa, AST_node *node, const int token_ptr, const char *msg); 

#endif