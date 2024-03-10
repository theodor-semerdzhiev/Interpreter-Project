#pragma once    
#include "rtobjects.h"


typedef struct Identifier Identifier;
typedef struct Identifier
{
    char *key;
    RtObject *obj;
    Identifier *next;
    AccessModifier access;
} Identifier;

typedef struct IdentifierTable
{
    Identifier **buckets;
    int bucket_count;
    int size;
} IdentTable;

IdentTable *init_IdentifierTable();
void Identifier_Table_add_var(IdentTable *table, const char *key, RtObject *obj, AccessModifier access);
RtObject *Identifier_Table_remove_var(IdentTable *table, const char *key);
RtObject *IdentifierTable_get(IdentTable *table, const char *key);
bool IdentifierTable_contains(IdentTable *table, const char *key);
int IdentifierTable_aggregate(IdentTable *table, const char* key);
RtObject **IdentifierTable_to_list(IdentTable *table);
Identifier **IdentifierTable_to_IdentList(IdentTable *table);
void free_IdentifierTable(IdentTable *table, bool free_rtobj);