#pragma once
#include <stdlib.h>
#include "rtobjects.h"

typedef struct MapNode MapNode;

typedef struct RtMap
{
    size_t size;        // number of elements in the set
    size_t bucket_size; // number of buckets
    MapNode **buckets;

    bool GCFlag; // used by GC
} RtMap;

RtMap *init_RtMap(unsigned long initial_bucket_size);
RtObject *rtmap_insert(RtMap *map, RtObject *key, RtObject *val);
RtObject *rtmap_remove(RtMap *map, RtObject *key);
RtObject *rtmap_get(const RtMap *map, RtObject *key);
RtObject **rtmap_getrefs(const RtMap *map, bool getkeys, bool getvals);
void rtmap_free(RtMap *map, bool free_keys, bool free_vals, bool free_immutable);
char *rtmap_toString(const RtMap *map);
void rtmap_print(const RtMap *map);
RtMap *rtmap_cpy(const RtMap *map, bool deepcpy_key, bool deepcpy_val, bool add_to_GC);
bool rtmap_equal(const RtMap *map1, const RtMap *map2);
RtMap *rtmap_clear(RtMap *map,  bool free_key, bool free_val, bool free_immutable);
