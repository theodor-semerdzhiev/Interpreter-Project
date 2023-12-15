#pragma once

#include "vartable.h"
#include "parser.h"

typedef enum scope_type
{
    GLOBAL_SCOPE,
    FUNCTION_SCOPE,
    OBJECT_SCOPE

} Scope;

typedef struct SemanticAnalyzer
{
    VarTable *symtable;
    Scope scope_type;
    bool is_in_loop;
    int nesting_lvl;

    struct _Lines
    {
        char **lines;
        int line_count;
    } lines;

    TokenList *token_list;

    char *filename;

} SemanticAnalyzer;

SemanticAnalyzer *malloc_semantic_analyser(const char *filename, char **lines, int line_num, TokenList *list);
void free_semantic_analyser(SemanticAnalyzer *sem_analyser);
bool exp_has_correct_semantics(SemanticAnalyzer *sem_analyser, ExpressionNode *root);
bool expression_component_has_correct_semantics(SemanticAnalyzer *sem_analyser, ExpressionComponent *node);
bool var_assignment_has_correct_semantics(SemanticAnalyzer *sem_analyser, AST_node *node);
bool check_argument_semantics(SemanticAnalyzer *sem_analyser, AST_node *node);
bool AST_list_has_consistent_semantics(SemanticAnalyzer *sem_analyser, AST_List *ast_list);
