#pragma once
#include <stdbool.h>

typedef struct ChainList ChainList;

typedef struct GenericSet
{

    size_t   size;

    size_t max_buckets;
    ChainList **buckets;

    bool (*is_equal)(const void *, const void *); // used to compare set elements
    unsigned int (*hash)(const void *);           // hash function
    void (*free_data)(void *);                    // free function for set elements

} GenericSet;

GenericSet *init_GenericSet(
    bool (*is_equal)(const void *, const void *),
    unsigned int (*hash)(const void *),
    void (*free_data)(void *));

bool GenericSet_has(const GenericSet *set, const void *data);
void *GenericSet_insert(GenericSet *set, void *data, bool free_duplicate_data);
void *GenericSet_remove(GenericSet *set, void *data);
void GenericSet_free(GenericSet *set, bool free_data);
void GenericSet_filter(GenericSet *set, bool (*filter)(void *), bool free_data);
void GenericSet_clear(GenericSet *set, bool free_val);
void GenericSet_print_contents(const GenericSet *map, void (*print_data)(const void *));
void **GenericSet_to_list(const GenericSet *set);
bool GenericSet_custom_find(const GenericSet *set, void *data, bool (*equal)(const void *, const void *));
