#pragma once

#include "parser.h"
#include "semanalysis.h"

void print_missing_operator_err(Parser *parser, const char *msg);
void print_missing_exp_component_err(Parser *parser, const char *msg);
void print_invalid_token_err(Parser *parser, const char *msg);
void print_expected_token_err(Parser *parser, const char *expected_token, const bool is_keyword, const char *msg);
void print_invalid_access_modifer_err(Parser *parser, const char *keyword, const char *msg);
void print_unexpected_end_of_file_err(Parser *parser, const char *msg);
void print_invalid_expression_component(Parser *parser, const char *msg);
void print_invalid_for_loop_exp(Parser *parser, const char *msg);
void print_invalid_exception_declaration(Parser *parser, const char *msg);


/* Semantic errors */
void print_undeclared_identifier_err(SemanticAnalyzer *sa, ExpressionComponent *comp, const char *msg);
void print_invalid_arg_identifier_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_access_modifier_semantics_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_object_block_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_terminal_top_component_err(SemanticAnalyzer *sa, ExpressionComponent *cm, const char *msg);
void print_invalid_func_call_err(SemanticAnalyzer *sa, ExpressionComponent *cm, const int token_ptr, const char *msg);
void print_invalid_ast_node(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_index_err(SemanticAnalyzer *sa, ExpressionComponent *cm, const int token_ptr, const char *msg);
void print_for_loop_ast_node_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_else_if_block_err(SemanticAnalyzer *sa, AST_node *node, const int token_ptr, const char *msg);
void print_invalid_else_block_err(SemanticAnalyzer *sa, AST_node *node, const int token_ptr, const char *msg);
void print_empty_exp_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_var_assignment_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_empty_body_err(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_arg_count_err(
    SemanticAnalyzer *sa,
    const int arg_count, const int expected_arg_count,
    const int token_ptr, const char *msg);
void print_invalid_global_return_value(SemanticAnalyzer *sa, const int token_ptr, const char *msg);
void print_invalid_try_catch(SemanticAnalyzer *sa, const int token_ptr, const char *msg);

    