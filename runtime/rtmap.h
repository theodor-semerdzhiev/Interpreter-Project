#pragma once
#include <stdlib.h>
#include "rtobjects.h"

typedef struct MapNode MapNode;

typedef struct RtMap {
    size_t size; // number of elements in the set
    size_t bucket_size; // number of buckets
    MapNode **buckets;
} RtMap;


RtMap *init_RtMap(unsigned long initial_bucket_size);
RtObject *rtmap_insert(RtMap *map, RtObject *key, RtObject *val);
RtObject *rtmap_remove(RtMap *map, RtObject *key);
RtObject *rtmap_get(const RtMap *map, RtObject *key);
RtObject **rtmap_getrefs(const RtMap *map, bool getkeys, bool getvals);
void rtmap_free(RtMap *map, bool free_keys, bool free_vals, bool free_immutable);
char *rtmap_toString(const RtMap *map);
RtMap *rtmap_cpy(const RtMap *map, bool deepcpy_key, bool deepcpy_val);
bool rtmap_equal(const RtMap *map1, const RtMap *map2);
