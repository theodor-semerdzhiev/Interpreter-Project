#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "vartable.h"
#include "parser.h"
#include "../runtime/builtins.h"

/**
 * DESCRIPTION:
 * This file contains the implementation of a specialized lookup table used exclusively by the 
 * Semantic Analyzer to keep track of bounded variables, and if needed their type.
*/

#define DEFAULT_BUCKET_SIZE 40

typedef struct VarChain VarChain;

static unsigned int hash(const char *ident);

/* Mallocs symbol struct */
static Variable *malloc_symbol(const char *ident, const char *filename)
{
    Variable *sym = malloc(sizeof(Variable));
    sym->ident = malloc_string_cpy(NULL, ident);
    sym->filename = malloc_string_cpy(NULL, filename);
    sym->next = NULL;
    sym->nesting_lvl = 0;
    return sym;
}

/* Inserts symbol into symbol chain */
static void insert_symbol(VarChain *chain, Variable *sym)
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
static VarChain *malloc_symbol_chain()
{
    VarChain *chain = malloc(sizeof(VarChain));
    chain->head = NULL;
    chain->tail = NULL;
    return chain;
}
/* Mallocs symbol table, inits the table to NULL */
VarTable *malloc_symbol_table()
{
    VarTable *symtable = malloc(sizeof(VarTable));
    symtable->bucket_count = DEFAULT_BUCKET_SIZE;
    symtable->sym_count = 0;
    symtable->table = malloc(sizeof(VarChain *) * DEFAULT_BUCKET_SIZE);
    memset(symtable->table, 0, sizeof(VarChain *) * DEFAULT_BUCKET_SIZE);
    for (int i = 0; i < DEFAULT_BUCKET_SIZE; i++)
    {
        symtable->table[i] = malloc_symbol_chain();
    }
    
    return symtable;
}

/* Frees symbol itself, does not touch *next node, make sure this function is called appropriately*/
void free_var_struct(Variable *sym)
{
    free(sym->ident);
    free(sym->filename);
    free(sym);
}

/* Frees the symbol table */
void free_var_table(VarTable *symtable)
{
    for (int i = 0; i < symtable->bucket_count; i++)
    {
        VarChain *chain = symtable->table[i];
        if (!chain)
            continue;
        Variable *head = chain->head;
        while (head)
        {
            Variable *tmp = head->next;
            free_var_struct(head);
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
bool add_var_to_vartable(
    VarTable *symtable,
    const char *ident,
    const char *filename,
    int nesting_lvl,
    VariableType symtype)
{
    unsigned int index = hash(ident);
    Variable *sym = malloc_symbol(ident, filename);
    sym->nesting_lvl = nesting_lvl;
    sym->type = symtype;

    VarChain *chain = symtable->table[index % symtable->bucket_count];

    insert_symbol(chain, sym);
    symtable->sym_count++;

    return true;
}

/**
 * 1- Checks if symbol is contained within the symbol table 
 * 2- Otherwise, check if its a built in function
 * */
bool vartable_has_var(VarTable *symtable, const char *ident)
{
    VarChain *chain = symtable->table[hash(ident) % symtable->bucket_count];
    Variable *head = chain->head;
    while (head)
    {
        if (!strcmp(head->ident, ident))
            return true;

        head = head->next;
    }

    return ident_is_builtin(ident);
}

/* Removes all symbols that have a smaller or equal nesting level */
void remove_all_vars_above_nesting_lvl(VarTable *symtable, int nesting_lvl)
{
    for (int i = 0; i < symtable->bucket_count; i++)
    {
        if (!symtable->table[i])
            continue;

        Variable *head = symtable->table[i]->head;
        while (head)
        {
            Variable *tmp = head->next;
            if (head->nesting_lvl >= nesting_lvl)
            {
                remove_var_from_vartable(symtable, head->ident, nesting_lvl);
            }

            head = tmp;
        }
    }
}

/* Removes symbol from table above or at nesting_lvl, return true if symbol was in table gets removes successfully, otherwise return false*/
bool remove_var_from_vartable(VarTable *symtable, const char *ident, const int nesting_lvl)
{
    unsigned int index = hash(ident) % symtable->bucket_count;
    VarChain *chain = symtable->table[index];
    Variable *head = chain->head;
    Variable *prev = NULL;

    while (head)
    {
        if (strcmp(head->ident, ident) == 0 && head->nesting_lvl >= nesting_lvl)
        {
            if (!prev)
            {
                chain->head = head->next;
                if (!head->next)
                    chain->tail = NULL;
            }
            else
            {
                prev->next = head->next;
                if (!head->next)
                    chain->tail = prev;
            }

            free_var_struct(head);
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