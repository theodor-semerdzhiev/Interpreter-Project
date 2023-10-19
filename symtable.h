#include <stdbool.h>

typedef struct symbol Symbol;

typedef struct symbol
{
    char *ident;
    int nesting_lvl;
    Symbol *next;
} Symbol;

struct SymbolChain
{
    Symbol *head;
    Symbol *tail;
};

typedef struct symtable
{
    struct SymbolChain **table;
    int sym_count;
    int bucket_count;
} SymbolTable;

void free_sym_table(SymbolTable *symtable);

void free_symbol_struct(Symbol *sym);
SymbolTable *malloc_symbol_table();

bool add_sym_to_symtable(SymbolTable *symtable, const char *ident, int nesting_lvl);

bool symtable_has_sym(SymbolTable *symtable, const char *ident);

void remove_all_syms_above_nesting_lvl(SymbolTable *symtable, int nesting_lvl);
bool remove_sym_from_symtable(SymbolTable *symtable, const char *ident);