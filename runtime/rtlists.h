#pragma once
#include "rtobjects.h"

#define DEFAULT_RTLIST_LEN 16

typedef struct RtList
{
    RtObject **objs;
    size_t length; // how many rt objects in the list

    size_t memsize; // keep track of size of array block
} RtList;

RtList *init_RtList(unsigned long initial_memsize);
RtObject *rtlist_append(RtList *list, RtObject *obj);
RtObject *rtlist_poplast(RtList *list);
RtObject *rtlist_remove(RtList *list, size_t index);
RtObject *rtlist_popfirst(RtList *list);
RtObject *rtlist_get(RtList *list, long index);
RtList *rtlist_cpy(RtList *list, bool deepcpy);
RtObject **rtlist_getrefs(const RtList *list);

bool rtlist_equals(RtList *l1, RtList *l2, bool deep_compare);
void rtlist_free(RtList *list);
char* rtlist_toString(RtList *list);

#define newDefaultList() init_RtList(DEFAULT_RTLIST_LEN)
