#pragma once
#include <stdbool.h>

typedef struct GenericMap GenericMap;

GenericMap *init_GenericMap(
    unsigned int (*hash)(const void *),
    bool (*are_keys_equal)(const void *, const void *),
    void (*free_key)(void *),
    void (*free_data)(void *));

bool GenericHashMap_contains_key(const GenericMap *map, void *key);
void *GenericHashMap_insert(GenericMap *map, void *key, void *value, bool free_duplicate_value);
void *GenericHashMap_get(GenericMap *map, void *key);
void *GenericHashmap_remove_key(GenericMap *map, void *key, bool free_key);
void map_filter_data(GenericMap *map, bool (*filter_data)(const void *), bool free_key, bool free_data);
void free_GenericMap(GenericMap *map, bool free_key, bool free_data);
