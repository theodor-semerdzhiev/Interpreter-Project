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
RtObject *rtset_get(const RtSet *set, RtObject *obj);
RtObject *rtset_remove(RtSet *set, RtObject *obj);
RtObject **rtset_getrefs(const RtSet *set);
void rtset_free(RtSet *set, bool free_obj, bool free_immutable);
char *rtset_toString(const RtSet *set);
RtSet *rtset_cpy(const RtSet *set, bool deepcpy);
bool rtset_equal(const RtSet *set1, const RtSet *set2);

