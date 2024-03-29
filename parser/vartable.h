#pragma once
#include <stdbool.h>

typedef enum VariableType
{
    SYMBOL_TYPE_FUNCTION,
    SYMBOL_TYPE_OBJECT,
    SYMBOL_TYPE_VARIABLE,
    SYMBOL_TYPE_EXCEPTION
} VariableType;

typedef struct variable Variable;

typedef struct variable
{
    char *ident;
    char *filename;
    unsigned int nesting_lvl;
    VariableType type;
    Variable *next;
} Variable;

struct VarChain
{
    Variable *head;
    Variable *tail;
};

typedef struct vartable
{
    struct VarChain **table;
    unsigned int sym_count;
    unsigned int bucket_count;
} VarTable;

void free_var_table(VarTable *symtable);

void free_var_struct(Variable *sym);
VarTable *malloc_symbol_table();

bool add_var_to_vartable(
    VarTable *symtable,
    const char *ident,
    const char *filename,
    int nesting_lvl,
    VariableType symtype);

bool vartable_has_var(VarTable *symtable, const char *ident);

void remove_all_vars_above_nesting_lvl(VarTable *symtable, unsigned int nesting_lvl);
bool remove_var_from_vartable(VarTable *symtable, const char *ident, unsigned int nesting_lvl);
