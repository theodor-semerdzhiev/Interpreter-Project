#pragma once
#include <stddef.h>
#include "rtobjects.h"

#define DEFAULT_RTLIST_LEN 16

typedef struct RtList
{
    RtObject **objs;
    size_t length; // how many rt objects in the list

    size_t memsize; // keep track of size of array block

    bool GCFlag; // used by garbage collector

    size_t refcount; // used by GC for ref counting
} RtList;

RtList *init_RtList(unsigned long initial_memsize);
RtObject *rtlist_append(RtList *list, RtObject *obj);
RtObject *rtlist_poplast(RtList *list);
RtObject *rtlist_removeindex(RtList *list, size_t index);
RtObject *rtlist_popfirst(RtList *list);
RtObject *rtlist_get(const RtList *list, long index);
RtList *rtlist_cpy(const RtList *list, bool deepcpy, bool add_to_GC);
RtList *rtlist_mult(const RtList *list, unsigned int number, bool add_to_GC);
RtList *rtlist_concat(const RtList *list1, const RtList *list2, bool cpy, bool add_to_GC);

RtObject **rtlist_getrefs(const RtList *list);
RtObject *rtlist_remove(RtList *list, RtObject *obj);

bool rtlist_equals(const RtList *l1, const RtList *l2, bool deep_compare);
RtList *rtlist_reverse(RtList *list);
bool rtlist_contains(const RtList *list, RtObject *obj);
void rtlist_free(RtList *list, bool free_refs, bool update_ref_counts);
void rtlist_print(const RtList *list);
char *rtlist_toString(const RtList *list);

#define newDefaultList() init_RtList(DEFAULT_RTLIST_LEN)
