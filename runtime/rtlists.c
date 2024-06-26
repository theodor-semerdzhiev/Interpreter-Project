#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "rtlists.h"
#include "gc.h"
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
    list->refcount = 0;
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

    // updates reference count
    rtobj_refcount_increment1(obj);
    
    // resizes array if needed
    if (list->length == list->memsize)
    {
        list->objs = realloc(list->objs, sizeof(RtObject *) * list->length * 2);
        if (!list->objs)
            return NULL;
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
    if(list->length == 0)
        return NULL;
    RtObject *obj = list->objs[--list->length];

    rtobj_refcount_decrement1(obj);

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
    return rtlist_removeindex(list, 0);
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
RtObject *rtlist_removeindex(RtList *list, size_t index)
{
    assert(list);
    if (index >= list->length)
        return NULL;

    RtObject *obj = list->objs[index];
    rtobj_refcount_decrement1(obj);

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
RtObject *rtlist_get(const RtList *list, long index)
{
    assert(list);
    if ((size_t)index >= list->length && (size_t)index < 0)
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
void rtlist_free(RtList *list, bool free_refs, bool update_ref_counts)
{
    for (size_t i = 0; i < list->length; i++) {
        
        if(update_ref_counts)
            rtobj_refcount_decrement1(list->objs[i]);

        if(free_refs) 
            rtobj_free(list->objs[i], false, update_ref_counts);        
    }

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
 * add_to_GC: wether objects CONTAINED by the list should be added to GC
 */
RtList *
rtlist_cpy(const RtList *list, bool deepcpy, bool add_to_GC)
{
    RtList *newlist = init_RtList(list->memsize);
    if (!newlist)
        return NULL;

    for (size_t i = 0; i < list->length; i++)
    {
        assert(list->objs[i]);
        RtObject *cpy = deepcpy ? rtobj_deep_cpy(list->objs[i], add_to_GC): list->objs[i];
        
        // this function will handle updating the reference count
        rtlist_append(newlist, cpy);

        if(add_to_GC)
            add_to_GC_registry(cpy);
    }

    return newlist;
}

/**
 * DESCRIPTION:
 * Helper function for multiplying a list, this function creates a new list
 * i.e
 * [1,2,3] * 3 = [1,2,3, 1,2,3, 1,2,3]
 * The rtobj_rt_preprocess function is used. Primitives type are always deep copied
 * 
 * PARAMS:
 * list: list to multiply
 * number: number to multiply by
 * add_to_GC: wether objects contained by new list should be added 
 * 
 * NOTE:
 * This function assumes that all elements in the input list are in the GC registry 
 * 
 * If list contains an other list, then that list is copied
*/
RtList *rtlist_mult(const RtList *list, unsigned int number, bool add_to_GC) {
    RtList *newlist = init_RtList(list->length * number);
    if(!newlist) {
        MallocError();
        return NULL;
    }

    if(number == 0)
        return newlist;
    
    for(unsigned int i = 0; i < number; i++) {
        for(size_t j = 0 ; j < list->length; j ++) {
            RtObject *obj = rtobj_rt_preprocess(list->objs[j], false, add_to_GC);

            // this function will handle updating the reference count
            rtlist_append(newlist, obj);
        }
    }

    return newlist;
}


/**
 * DESCRIPTION:
 * This function takes 2 lists and appends them together. This function CREATES a new list
 * 
 * NOTE:
 * This function runs rtobj_rt_preprocess if cpy = true, before adding obj into the new list
 * Therefore, primitive types (Number, Undefined, Null) are always gonna be deep copied
 * 
 * PARAMS:
 * list1: 
 * list2:
 * cpy: wether rtobj_rt_preprocess should be run before inserting object into new list
 * add_to_GC: wether elements in the new list should be added to the GC
*/
RtList *rtlist_concat(const RtList *list1, const RtList *list2, bool cpy, bool add_to_GC) {
    assert(list1 && list2);

    RtList *newlist = init_RtList(list1->length + list2->length);
    
    if(!newlist) {
        MallocError();
        return NULL;
    }

    for(size_t i=0; i < list1->length; i++) {
        RtObject *obj = list1->objs[i];
        if(cpy)
            obj = rtobj_rt_preprocess(list1->objs[i], false, add_to_GC);

        // this function will handle updating the reference count
        rtlist_append(newlist, obj);
    }

    for(size_t i=0; i < list2->length; i++) {
        RtObject *obj = list2->objs[i];

        if(cpy)
            obj = rtobj_rt_preprocess(list2->objs[i], false, add_to_GC);

        // this function will handle updating the reference count
        rtlist_append(newlist, obj);
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
bool rtlist_equals(const RtList *l1, const RtList *l2, bool deep_compare)
{
    if (l1->length != l2->length)
        return false;

    for (size_t i = 0; i < l1->length; i++)
    {
        if (deep_compare && !rtobj_equal(l1->objs[i], l2->objs[i]))
            return false;
        
        if (!deep_compare && l1->objs[i] != l2->objs[i])
            return false;
    }
    return true;
}

/**
 * DESCRIPTION:
 * Useful function for checking if a object is contained within a list
 *
 * PARAMS:
 * list: list
 * obj: obj
 */
bool rtlist_contains(const RtList *list, RtObject *obj)
{
    assert(list && obj);

    for (size_t i = 0; i < list->length; i++)
    {
        if (rtobj_equal(list->objs[i], obj))
            return true;
    }
    return false;
}

/**
 * DESCRIPTION:
 * Performs a reverse operation on the list
*/
RtList *rtlist_reverse(RtList *list) {
    assert(list);
    for(size_t i =0; i < (list->length/2); i++) {
        RtObject *tmp= list->objs[i];
        list->objs[i] = list->objs[list->length-i -1];
        list->objs[list->length-i -1] = tmp;
    }
    return list;
}

/**
 * DESCRIPTION:
 * Removes first occurence of object in list, and returns the matching object.
 * If not match is found then NULL is returned
 * PARAMS:
 * list: list
 * obj: obj
 */
RtObject *rtlist_remove(RtList *list, RtObject *obj)
{
    assert(list && obj);
    for (size_t i = 0; i < list->length; i++) {
        if (rtobj_equal(list->objs[i], obj)) {
            rtlist_removeindex(list, i);
            return list->objs[i];
        }
    }
    return NULL;
}

/**
 * DESCRIPTION:
 * Prints out list runtime object to standard output
*/
void rtlist_print(const RtList *list) {
    assert(list);

    printf("[");
    for (size_t i = 0; i < list->length; i++) {
        char *obj_to_str = rtobj_toString(list->objs[i]);
        if (list->objs[i]->type == STRING_TYPE)
            printf("\"%s\"", obj_to_str);
        else
            printf("%s", obj_to_str);
        
        free(obj_to_str);
        if(i + 1 == list->length) 
            break;
        
        printf(", ");
    }
    printf("]");
}


__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Converts list to string
*/
char* rtlist_toString(const RtList *list) {
    assert(list);
    char buffer[100];
    buffer[0]='\0';
    snprintf(buffer, 100, "list@%p", list);
    char *cpy = cpy_string(buffer);
    if(!cpy) MallocError();
    return cpy;
}

__attribute__((warn_unused_result))
/**
 * **DEPRECATED**
 * DESCRIPTION:
 * Converts a list object to a human readable string
 *
 *
 */
char *
_rtlist_toString(const RtList *list)
{
    assert(list);
    char *str = cpy_string("[");
    if (!str)
        return NULL;

    for (size_t i = 0; i < list->length; i++)
    {
        char *obj_to_str = rtobj_toString(list->objs[i]);

        // if its a string
        if (list->objs[i]->type == STRING_TYPE)
        {
            char *tmp = concat_strings("\"", obj_to_str);
            free(obj_to_str);
            char *tmp1 = concat_strings(tmp, "\"");
            free(tmp);
            obj_to_str = tmp1;
        }

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
