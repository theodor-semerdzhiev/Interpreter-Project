#include "parser.h"


struct symtable;

void free_sym_table(struct symtable *table);

void free_symbol_struct(struct symbol *sym);  

void add_sym_to_symtable(struct symtable *table, const char* ident);

bool symtable_has_sym(struct symtable *table, const char* ident);

bool remove_sym_from_symtable(struct symtable *table, const char* ident);