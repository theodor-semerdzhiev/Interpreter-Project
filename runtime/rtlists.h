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
} RtList;

RtList *init_RtList(unsigned long initial_memsize);
RtObject *rtlist_append(RtList *list, RtObject *obj);
RtObject *rtlist_poplast(RtList *list);
RtObject *rtlist_removeindex(RtList *list, size_t index);
RtObject *rtlist_popfirst(RtList *list);
RtObject *rtlist_get(RtList *list, long index);
RtList *rtlist_cpy(RtList *list, bool deepcpy);
RtObject **rtlist_getrefs(const RtList *list);
RtObject *rtlist_remove(RtList *list, RtObject *obj);

bool rtlist_equals(RtList *l1, RtList *l2, bool deep_compare);
RtList *rtlist_reverse(RtList *list);
bool rtlist_contains(RtList *list, RtObject *obj);
void rtlist_free(RtList *list, bool free_refs);
void rtlist_print(const RtList *list);
char *rtlist_toString(const RtList *list);

#define newDefaultList() init_RtList(DEFAULT_RTLIST_LEN)
