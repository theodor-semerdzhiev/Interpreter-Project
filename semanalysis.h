#include "symtable.h"

typedef enum scope_type
{
    GLOBAL_SCOPE,
    FUNCTION_SCOPE

} Scope;

typedef struct SemanticAnalyser
{
    SymbolTable *symtable;
    Scope scope_type;
    bool is_in_loop;
    int nesting_lvl;

} SemanticAnalyser;