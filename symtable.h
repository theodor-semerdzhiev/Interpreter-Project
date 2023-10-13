#include "parser.h"

typedef struct symbol
{
    char *ident;
    struct symbol *next;
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

void add_sym_to_symtable(SymbolTable *symtable, const char *ident);

bool symtable_has_sym(SymbolTable *symtable, const char *ident);

bool remove_sym_from_symtable(SymbolTable *symtable, const char *ident);