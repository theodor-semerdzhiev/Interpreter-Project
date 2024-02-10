#pragma once
#include <stdlib.h>
#include "rtobjects.h"

typedef struct MapNode MapNode;

typedef struct RtSet
{
    size_t size;        // number of elements in the set
    size_t bucket_size; // number of buckets
    MapNode **buckets;

    bool GCFlag; // used by GC
} RtSet;

RtSet *init_RtMap(unsigned long initial_bucket_size);
RtObject *rtmap_insert(RtSet *map, RtObject *key, RtObject *val);
RtObject *rtmap_remove(RtSet *map, RtObject *key);
RtObject *rtmap_get(const RtSet *map, RtObject *key);
RtObject **rtmap_getrefs(const RtSet *map, bool getkeys, bool getvals);
void rtmap_free(RtSet *map, bool free_keys, bool free_vals, bool free_immutable);
char *rtmap_toString(const RtSet *map);
RtSet *rtmap_cpy(const RtSet *map, bool deepcpy_key, bool deepcpy_val);
bool rtmap_equal(const RtSet *map1, const RtSet *map2);
