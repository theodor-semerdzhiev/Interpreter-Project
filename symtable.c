#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"
#include "parser.h"

#define DEFAULT_BUCKET_SIZE 40

typedef struct SymbolChain SymbolChain;

static unsigned int hash(const char *ident);

/* Mallocs symbol struct */
static Symbol *malloc_symbol(const char *ident)
{
    Symbol *sym = malloc(sizeof(Symbol));
    sym->ident = malloc_string_cpy(NULL, ident);
    sym->next = NULL;
    sym->nesting_lvl=0;
    return sym;
}

/* Inserts symbol into symbol chain */
static void insert_symbol(SymbolChain *chain, Symbol *sym)
{
    if (!chain->head)
    {
        chain->head = sym;
        chain->tail = sym;
    }
    else
    {
        chain->tail->next = sym;
        chain->tail = sym;
    }
}

/* Mallocs struct for hashmap chaining */
static SymbolChain *malloc_symbol_chain() {
    SymbolChain *chain = malloc(sizeof(SymbolChain));
    chain->head=NULL;
    chain->tail=NULL;
    return chain;
} 
/* Mallocs symbol table, inits the table to NULL */
SymbolTable *malloc_symbol_table()
{
    SymbolTable *symtable = malloc(sizeof(SymbolTable));
    symtable->bucket_count = DEFAULT_BUCKET_SIZE;
    symtable->sym_count = 0;
    symtable->table = malloc(sizeof(SymbolChain *) * DEFAULT_BUCKET_SIZE);
    memset(symtable->table, 0, sizeof(SymbolChain *) * DEFAULT_BUCKET_SIZE);
    for(int i=0; i < DEFAULT_BUCKET_SIZE; i++) {
        symtable->table[i] = malloc_symbol_chain();

    }
    return symtable;
}

/* Frees symbol itself, does not touch *next node, make sure this function is called appropriately*/
void free_symbol_struct(Symbol *sym)
{
    free(sym->ident);
    free(sym);
}

/* Frees the symbol table */
void free_sym_table(SymbolTable *symtable)
{
    for (int i = 0; i < symtable->bucket_count; i++)
    {
        SymbolChain *chain = symtable->table[i];
        if(!chain) continue;
        Symbol *head = chain->head;
        while (head)
        {
            Symbol *tmp = head->next;
            free_symbol_struct(head);
            head = tmp;
        }
        free(chain);
    }
    free(symtable->table);
    free(symtable);
}

/* Adds symbol to symbol table, 
- returns true if it was added successfully
- return false if the same Symbol is already in the table */
bool add_sym_to_symtable(SymbolTable *symtable, const char *ident, int nesting_lvl)
{
    if(symtable_has_sym(symtable, ident)) return false;

    unsigned int index = hash(ident);
    Symbol *sym = malloc_symbol(ident);
    sym->nesting_lvl=nesting_lvl;

    SymbolChain *chain = symtable->table[index % symtable->bucket_count];

    insert_symbol(chain, sym);
    symtable->sym_count++;

    return true;
}

/* Checks if symbol is contained within the symbol table */
bool symtable_has_sym(SymbolTable *symtable, const char *ident)
{
    SymbolChain *chain = symtable->table[hash(ident) % symtable->bucket_count];
    Symbol *head = chain->head;
    while (head)
    {
        if (!strcmp(head->ident, ident))
            return true;

        head = head->next;
    }
    return false;
}

/* Removes all symbols that have a smaller or equal nesting level */
void remove_all_syms_above_nesting_lvl(SymbolTable *symtable, int nesting_lvl) {
    for(int i=0; i < symtable->bucket_count; i++) {
        if(!symtable->table[i]) continue;
        
        Symbol *head = symtable->table[i]->head;
        while(head) {
            Symbol *tmp = head->next;
            if(head->nesting_lvl >= nesting_lvl) {
                remove_sym_from_symtable(symtable, head->ident);
            }
            
            head=tmp;
            
        }
    }
}


/* Removes symbol from table, return true if symbol was in table gets removes successfully, otherwise return false*/
bool remove_sym_from_symtable(SymbolTable *symtable, const char *ident)
{
    unsigned int index = hash(ident) % symtable->bucket_count;
    SymbolChain *chain = symtable->table[index];
    Symbol *head = chain->head;
    Symbol *prev = NULL;

    while (head)
    {
        if (strcmp(head->ident, ident) == 0)
        {
            if (!prev)
            {
                chain->head = head->next;
                if (!head->next)
                    chain->tail = NULL;
                
            } else {
                 prev->next = head->next;
                if (!head->next)
                    chain->tail = prev;
            }

            free_symbol_struct(head);
            return true;
        }
        prev = head;
        head = head->next;
    }
    return false;
}


// hash function: string -> int
static unsigned int hash(const char *ident)
{
    unsigned int hash = 7; // prime number

    for (int i = 0; i < (int)strlen(ident); i++)
    {
        hash = hash * 31 + ident[i];
    }
    return hash;
}