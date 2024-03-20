#pragma once
#include <stdlib.h>

typedef struct SetNode SetNode;

typedef struct RtSet
{
    size_t size;
    size_t bucket_size;

    SetNode **buckets;

    bool GCFlag;
} RtSet;


RtSet *init_RtSet(size_t max_buckets);
RtObject *rtset_insert(RtSet *set, RtObject *val);
RtObject *rtset_get(const RtSet *set, const RtObject *obj);
RtObject *rtset_remove(RtSet *set, RtObject *obj);
RtObject **rtset_getrefs(const RtSet *set);
void rtset_free(RtSet *set, bool free_obj, bool free_immutable);
void rtset_print(const RtSet *set);
char *rtset_toString(const RtSet *set);
RtSet *rtset_cpy(const RtSet *set, bool deepcpy, bool add_to_GC);
bool rtset_equal(const RtSet *set1, const RtSet *set2);
RtSet *rtset_clear(RtSet *set, bool free_obj, bool free_immutable); 
RtSet *rtset_intersection(const RtSet *set1, const RtSet *set2, bool cpy, bool add_to_GC);
RtSet *rtset_union(const RtSet *set1, const RtSet *set2, bool cpy, bool add_to_GC);

