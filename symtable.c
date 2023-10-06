#include <stdbool.h>
#include <stdlib.h>

// Store a symbol 
static struct symbol {
    char *name; // symbol name 
    struct symbol *next; // next symbol (used for chaining)
};

struct symtable {
    int size; // # Of symbol in table
    int buckets; // # max number of current buckets
    
    struct symbol **map;

};

static unsigned int hash(const char* ident);

/* Frees symbol itself, does not touch *next node, make sure this function is called appropriately*/
void free_symbol_struct(struct symbol *sym) {
    free(sym->name);
    free(sym);
}

/* Frees symbol table */
void free_sym_table(struct symtable *table) {
    for(int i=0; i < table->size; i++) {
        struct symbol *ptr = table->map[i];
        while(ptr) {
            struct symbol *tmp = ptr->next;

            free_symbol_struct(ptr);
            ptr = tmp;
        }
    }

    free(table->buckets);
    free(table);
}


//TODO
/* Adds symbol to table */
void add_sym_to_symtable(struct symtable *table, const char* ident) {

}

//TODO
/* Checks if symbol is in table */
bool symtable_has_sym(struct symtable *table, const char* ident) {

}

/* Removes symbol from table, return true if symbol was in table gets removes successfully, otherwise return false*/
bool remove_sym_from_symtable(struct symtable *table, const char* ident) {
    return false;
}   


// hash function: string -> int
static unsigned int hash(const char* ident) {
  int hash=7; //prime number

  for(int i=0; i < (int)strlen(ident); i++) {
    hash = hash*31 + ident[i];
  }
  return hash;
}