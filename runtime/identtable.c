#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "rtobjects.h"
#include "identtable.h"
#include "gc.h"
#include "../generics/utilities.h"

/**
 * DESCRIPTION:
 * Below is the implementation for the variable lookup table built into call frames
 */


#define DEFAULT_IDENT_TABLE_BUCKET_SIZE 64

/**
 * Creates a new Identifer node for Identifier Table
 */
static Identifier *init_Identifier(const char *varname, RtObject *obj, AccessModifier access)
{
    Identifier *node = malloc(sizeof(Identifier));
    if (!node)
        return NULL;
    node->obj = obj;
    rtobj_refcount_increment1(obj);
    node->key = cpy_string(varname);
    node->access = access;
    node->next = NULL;
    return node;
}

/**
 * Frees identifier node
 * free_rtobj: wether Runtime Object should also be freed
 *
 * NOTE:
 * if free_rtobj is set to false, the runtime object itself will be freed only
 */
static void free_Identifier(Identifier *node, bool free_rtobj)
{
    if (!node)
        return;
    free(node->key);
    rtobj_refcount_decrement1(node->obj);
    if (free_rtobj)
    {
        remove_from_GC_registry(node->obj, true);
    }

    free(node);
}

/**
 * Creates a new Identifier Table
 *
 */
IdentTable *init_IdentifierTable()
{
    IdentTable *table = malloc(sizeof(IdentTable));
    if (!table)
        return NULL;
    table->bucket_count = DEFAULT_IDENT_TABLE_BUCKET_SIZE;
    table->buckets = calloc(table->bucket_count, sizeof(Identifier *));
    if (!table->buckets)
    {
        free(table);
        return NULL;
    }
    table->size = 0;
    return table;
}

/**
 * Maps variable name to Runtime Object
 */
void Identifier_Table_add_var(IdentTable *table, const char *key, RtObject *obj, AccessModifier access)
{
    assert(table);
    unsigned int index = djb2_string_hash(key) % table->bucket_count;
    if (!table->buckets[index])
    {
        table->buckets[index] = init_Identifier(key, obj, access);
    }
    else
    {
        Identifier *node = init_Identifier(key, obj, access);
        node->next = table->buckets[index];
        table->buckets[index] = node;
    }
    table->size++;
}

/**
 * Removes variable from table, and returns the mapped RunTime object
 * If variable is not present, then function return NULL
 */
RtObject *Identifier_Table_remove_var(IdentTable *table, const char *key)
{
    assert(table);
    unsigned int index = djb2_string_hash(key) % table->bucket_count;

    Identifier *node = table->buckets[index];
    Identifier *prev = NULL;
    while (node)
    {
        if (strings_equal(node->key, key))
        {
            RtObject *obj = node->obj;
            if (!prev)
            {
                table->buckets[index] = node->next;
                free_Identifier(node, false);
                table->size--;
                return obj;
            }
            else
            {
                prev->next = node->next;
                free_Identifier(node, false);
                table->size--;
                return obj;
            }
        }

        prev = node;
        node = node->next;
    }

    return NULL;
}

/**
 * Fetches the object associated with the key, returns NULL if key does not exist
 */
RtObject *IdentifierTable_get(IdentTable *table, const char *key)
{
    unsigned int index = djb2_string_hash(key) % table->bucket_count;
    if (!table->buckets[index])
        return NULL;

    Identifier *node = table->buckets[index];
    while (node)
    {
        if (strings_equal(node->key, key))
            return node->obj;
        node = node->next;
    }
    return NULL;
}

/**
 * Checks if Identifier table contains key mapping
 */
bool IdentifierTable_contains(IdentTable *table, const char *key)
{
    unsigned int index = djb2_string_hash(key) % table->bucket_count;
    if (!table->buckets[index])
        return false;

    Identifier *node = table->buckets[index];
    while (node)
    {
        if (strings_equal(node->key, key))
            return true;
        node = node->next;
    }
    return false;
}

/**
 * DESCRIPTION:
 * Takes all elements of the table and puts them in a list (NULL terminated)
 *
 * NOTE:
 * Function returns NULL if malloc fails
 */
RtObject **IdentifierTable_to_list(IdentTable *table)
{
    RtObject **list = malloc(sizeof(RtObject *) * (table->size + 1));
    if (!list)
        return NULL;

    unsigned int count = 0;
    for (int i = 0; i < table->bucket_count; i++)
    {
        if (!table->buckets[i])
            continue;

        Identifier *ptr = table->buckets[i];
        while (ptr)
        {
            list[count++] = ptr->obj;
            ptr = ptr->next;
        }
    }
    list[table->size] = NULL;
    return list;
}

/**
 * DESCRIPTION:
 * Takes all Identifier nodes in the table puts them in a NULL terminated list
 */
Identifier **IdentifierTable_to_IdentList(IdentTable *table)
{
    assert(table);
    Identifier **list = malloc(sizeof(RtObject *) * (table->size + 1));
    if (!list)
        return NULL;

    unsigned int count = 0;
    for (int i = 0; i < table->bucket_count; i++)
    {
        if (!table->buckets[i])
            continue;

        Identifier *ptr = table->buckets[i];
        while (ptr)
        {
            list[count++] = ptr;
            ptr = ptr->next;
        }
    }

    list[table->size] = NULL;
    return list;
}

/**
 * Counts the number of Identifier Mappings contained within map
 */
int IdentifierTable_aggregate(IdentTable *table, const char *key)
{
    unsigned int index = djb2_string_hash(key) % table->bucket_count;
    if (!table->buckets[index])
        return 0;

    Identifier *node = table->buckets[index];
    int count = 0;
    while (node)
    {
        if (strings_equal(node->key, key))
            count++;
        node = node->next;
    }
    return count;
}

/**
 * Frees Identifier Table
 */
void free_IdentifierTable(IdentTable *table, bool free_rtobj)
{
    if (!table)
        return;

    for (int i = 0; i < table->bucket_count; i++)
    {
        Identifier *ptr = table->buckets[i];
        while (ptr)
        {
            Identifier *next = ptr->next;
            free_Identifier(ptr, free_rtobj);
            ptr = next;
        }
    }
    free(table->buckets);
    free(table);
}
