#include "parser.h"

void print_missing_operator_err(Parser *parser);
void print_missing_exp_component_err(Parser *parser);
void print_invalid_token_err(Parser *parser);
void print_expected_token_err(Parser *parser, const char* expected_token, const bool is_keyword);
void print_invalid_access_modifer_err(Parser *parser, const char* keyword);