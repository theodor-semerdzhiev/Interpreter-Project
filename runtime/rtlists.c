#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "rtlists.h"
#include "rtobjects.h"
#include "../generics/utilities.h"

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Initializes a new runtime list with a initial memory size
 *
 * PARAMS:
 * initial_memsize: intial size of array block
 */
RtList *
init_RtList(unsigned long initial_memsize)
{
    RtList *list = malloc(sizeof(RtList));
    if (!list)
        return NULL;
    list->memsize = initial_memsize;
    list->length = 0;
    list->objs = malloc(sizeof(RtObject *) * (initial_memsize));
    if (!list->objs)
    {
        free(list);
        return NULL;
    }
    for (unsigned long i = 0; i < initial_memsize; i++)
    {
        list->objs[i] = NULL;
    }
    return list;
}

/**
 * DESCRIPTION:
 * Pushes a runtime objects to runtime list and returns the object
 *
 * PARAMS:
 * list: list to push to
 * obj: object to add
 *
 * NOTE:
 * If resizing fails, function WILL return NULL;
 */
RtObject *rtlist_append(RtList *list, RtObject *obj)
{
    assert(list && obj);
    list->objs[list->length++] = obj;

    // resizes array if needed
    if (list->length == list->memsize)
    {
        list->objs = realloc(list->objs, sizeof(RtObject *) * list->length * 2);
        if (!list->objs)
            return NULL;
        list->length *= 2;
        list->memsize *= 2;
    }

    return obj;
}

/**
 * DESCRIPTION:
 * Pops a runtime objects the from list and returns the object
 *
 * PARAMS:
 * list: list to push to
 * obj: object to add
 *
 * NOTE:
 * If list is empty function will return NULL
 */
RtObject *rtlist_poplast(RtList *list)
{
    assert(list);
    RtObject *obj = list->objs[list->length--];

    // resizes array if needed
    if (list->length == list->memsize / 2 && list->length > DEFAULT_RTLIST_LEN)
    {
        list->objs = realloc(list->objs, sizeof(RtObject *) * list->length / 2);
        list->memsize /= 2;
    }

    return obj;
}

/**
 * DESCRIPTION:
 * Removes the first element of a list
 *
 * PARAMS:
 * list: list
 *
 * NOTE:
 * This function is basically just wrapper for the remove function
 */
RtObject *rtlist_popfirst(RtList *list)
{
    assert(list);
    return rtlist_remove(list, 0);
}

/**
 * DESCRIPTION:
 * Removes a object from a list using its index, and makes sure the list's integrity stays intact
 *
 * PARAMS:
 * list: runtime list
 * index: index at which to remove from
 *
 * NOTE:
 * if index is out of bounds, function will return NULL
 */
RtObject *rtlist_remove(RtList *list, size_t index)
{
    assert(list);
    if (index >= list->length)
        return NULL;
    RtObject *obj = list->objs[index];
    for (size_t i = index + 1; i < list->length; i++)
    {
        list->objs[i - 1] = list->objs[i];
    }
    list->length--;

    // resizes array if needed
    if (list->length == list->memsize / 2 && list->length > DEFAULT_RTLIST_LEN)
    {
        list->objs = realloc(list->objs, sizeof(RtObject *) * list->length / 2);
        list->memsize /= 2;
    }
    return obj;
}

/**
 * DESCRIPTION:
 * Gets certain object at a certain index in a list
 *
 * PARAMS:
 * list: list
 * index: index
 *
 * NOTES:
 * This function will return NULL if the index is out of bounds
 */
RtObject *rtlist_get(RtList *list, long index)
{
    assert(list);
    if ((size_t)index >= list->length)
        return NULL;
    else
        return list->objs[index];
}

/**
 * DESCRIPTION:
 * Frees runtime list
 *
 * NOTE:
 * Does NOT free objects inside list
 */
void rtlist_free(RtList *list)
{
    free(list->objs);
    free(list);
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Copies runtime list, either a deep copy or shallow copy
 *
 * PARAMS:
 * list: list to copy
 * deepcpy: wether the list should be deep copied or shallow copied
 */
RtList *
rtlist_cpy(RtList *list, bool deepcpy)
{
    RtList *newlist = init_RtList(list->memsize);
    if (newlist)
        return NULL;
    for (size_t i = 0; i < list->memsize; i++)
    {
        newlist->objs[i] = deepcpy ? rtobj_deep_cpy(list->objs[i]) : list->objs[i];
    }

    return newlist;
}

/**
 * DESCRIPTION:
 * Creates a NULL termianted array of all objects in the rt list, this is used by the garbage collector
 *
 * NOTE:
 * returns NULL pointer if allocation fails. If list length is 1, then it will allocate a single word with a NULL value.
 */
RtObject **rtlist_getrefs(const RtList *list)
{
    assert(list);
    RtObject **refs = malloc(sizeof(RtObject *) * (list->length + 1));
    if (!refs)
        return NULL;
    for (size_t i = 0; i < list->length; i++)
    {
        refs[i] = list->objs[i];
    }
    refs[list->length] = NULL;
    return refs;
}

/**
 * DESCRIPTION:
 * Checks if 2 lists are equal
 *
 * PARAMS:
 * l1: list 1
 * l2: list 2
 * deep_compare: wether they should be compared by reference only or not
 */
bool rtlist_equals(RtList *l1, RtList *l2, bool deep_compare)
{
    if (l1->length != l2->length)
        return false;

    for (size_t i = 0; i < l1->length; i++)
    {
        if (deep_compare && !rtobj_equal(l1->objs[i], l2->objs[i]))
        {
            return false;
        }
        else if (!deep_compare && l1->objs[i] != l2->objs[i])
        {
            return false;
        }
    }
    return true;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Converts a list object to a human readable string
 *
 *
 */
char *
rtlist_toString(RtList *list)
{
    assert(list);
    char *str = cpy_string("[");
    if (!str)
        return NULL;

    for (size_t i = 0; i < list->length; i++)
    {
        char *obj_to_str = rtobj_toString(list->objs[i]);
        if (i + 1 == list->length)
        {
            char *tmp = concat_strings(str, obj_to_str);
            free(obj_to_str);
            free(str);
            str = tmp;
            break;
        }

        char *tmp = concat_strings(str, obj_to_str);
        free(obj_to_str);
        free(str);
        str = concat_strings(tmp, ", ");
        free(tmp);
    }

    char *tmp = concat_strings(str, "]");
    if (!tmp)
        return NULL;
    free(str);
    str = tmp;
    return str;
}