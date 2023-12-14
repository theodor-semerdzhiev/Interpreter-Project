#ifndef GENERIC_MAP_H
#define GENERIC_MAP_H

#include <stdbool.h>

typedef struct ChainList ChainList;

typedef struct GenericSet
{

    int size;

    int max_buckets;
    ChainList **buckets;

    bool (*is_equal)(const void *, const void *); // used to compare set elements
    unsigned int (*hash)(const void *);           // hash function
    void (*free_data)(void *);                    // free function for set elements

} GenericSet;

GenericSet *init_GenericSet(
    bool (*is_equal)(void *, void *),
    unsigned int (*hash)(void *),
    void (*free_data)(void *));

bool set_contains(const GenericSet *set, const void *data);
void *set_insert(GenericSet *set, void *data);
void *set_remove(GenericSet *set, void *data);
void free_GenericSet(GenericSet *set, bool free_data);
void set_filter_remove(GenericSet *set, bool (*filter)(void *), bool free_data);
void GenericSet_print_contents(const GenericSet *map, void (*print_data)(const void*));
void** GenericSet_to_list(const GenericSet *set);

#endif