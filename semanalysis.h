#include "symtable.h"
#include "parser.h"

typedef enum scope_type
{
    GLOBAL_SCOPE,
    FUNCTION_SCOPE,
    OBJECT_SCOPE

} Scope;

typedef struct SemanticAnalyser
{
    SymbolTable *symtable;
    Scope scope_type;
    bool is_in_loop;
    int nesting_lvl;

} SemanticAnalyser;

SemanticAnalyser *malloc_semantic_analyser();
void free_semantic_analyser(SemanticAnalyser *sem_analyser);
bool exp_has_correct_semantics(SemanticAnalyser *sem_analyser, ExpressionNode *root);
bool expression_component_has_correct_semantics(SemanticAnalyser *sem_analyser, ExpressionComponent *node);
bool var_assignment_has_correct_semantics(SemanticAnalyser *sem_analyser, AST_node *node);
bool func_declaration_arg_semantics(SemanticAnalyser *sem_analyser, AST_node *node);
bool AST_list_has_consistent_semantics(SemanticAnalyser *sem_analyser, AST_List *ast_list);