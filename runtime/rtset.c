#include <assert.h>
#include "../generics/utilities.h"
#include "rtobjects.h"
#include "rtset.h"
#include "gc.h"

/**
 * DESCRIPTION:
 * This file contains runtime set implementation
 */

typedef struct SetNode SetNode;

typedef struct SetNode
{
    RtObject *obj;
    SetNode *next;
} SetNode;

#define GetIndexedHash(set, obj) rtobj_hash(obj) % set->bucket_size;

#define DOWNSIZING_THRESHOLD 64
#define DEFAULT_SET_BUCKETS 16

/**
 * DESCRIPTION:
 * Helper for creating a Set chaining node with RtObject
 */
static SetNode *init_SetNode(RtObject *obj)
{
    SetNode *node = malloc(sizeof(SetNode));
    if (!node)
    {
        MallocError();
        return NULL;
    }
    node->obj = obj;
    node->next = NULL;
    return node;
}

/**
 * DESCRIPTION:
 * Frees Set node
 *
 * PARAMS:
 * node: Map Node to free
 * free_val: wether the associated object should be freed
 * update_ref_counts: wether references counts should be updated
 */
static void free_SetNode(SetNode *node, bool free_val, bool free_immutable, bool update_ref_counts)
{
    if (!node)
        return;

    if (free_val)
        rtobj_free(node->obj, free_immutable, update_ref_counts);

    free(node);
}

/**
 * DESCRIPTION:
 * Initializes set
 */
RtSet *init_RtSet(size_t max_buckets)
{
    RtSet *set = malloc(sizeof(RtSet));
    if (!set) {
        MallocError();
        return NULL;
    }

    set->bucket_size = max_buckets > DEFAULT_SET_BUCKETS ? max_buckets : DEFAULT_SET_BUCKETS;
    set->buckets = calloc(set->bucket_size, sizeof(SetNode *));
    if (!set->buckets)
    {
        free(set);
        MallocError();
        return NULL;
    }

    set->size = 0;
    set->refcount = 0;
    return set;
}

/**
 * DESCRIPTION:
 * Helper for resizing the number of buckets by factor of 2 when the size of the set gets too big compared to the max buckets
 *
 * NOTE:
 * This function returns the modified input set, it will return NULL, if malloc fails
 * PARAMS:
 * set: set to be reisized
 * fact
 */
static RtSet *rtset_resize(RtSet *set)
{
    assert(set);
    SetNode **resized = calloc(set->bucket_size * 2, sizeof(SetNode *));
    if (!resized)
        return NULL;

    size_t old_bucket_size = set->bucket_size;
    set->bucket_size *= 2;
    for (size_t i = 0; i < old_bucket_size; i++)
    {
        if (!set->buckets[i])
            continue;

        SetNode *ptr = set->buckets[i];
        while (ptr)
        {
            SetNode *tmp = ptr->next;
            unsigned int index = GetIndexedHash(set, ptr->obj);
            if (!resized[index])
            {
                ptr->next = NULL;
                resized[index] = ptr;
            }
            else
            {
                ptr->next = resized[index];
                resized[index] = ptr;
            }

            ptr = tmp;
        }
    }

    free(set->buckets);
    set->buckets = resized;
    return set;
}

/**
 * DESCRIPTION:
 * Helper for downsizing the number of buckets when the size of the set is too small compared to the number of buckets
 *
 * NOTE:
 * This function returns the original set
 *
 * PARAMS:
 * set: set to be downsized
 */
static RtSet *rtset_downsize(RtSet *set)
{
    assert(set);
    SetNode **resized = calloc(set->bucket_size / 2, sizeof(SetNode *));
    if (!resized)
        return NULL;

    size_t old_bucket_size = set->bucket_size;
    set->bucket_size /= 2;
    for (size_t i = 0; i < old_bucket_size; i++)
    {
        if (!set->buckets[i])
            continue;

        SetNode *ptr = set->buckets[i];
        while (ptr)
        {
            SetNode *tmp = ptr->next;
            unsigned int index = GetIndexedHash(set, ptr->obj);
            if (!resized[index])
            {
                ptr->next = NULL;
                resized[index] = ptr;
            }
            else
            {
                ptr->next = resized[index];
                resized[index] = ptr;
            }

            ptr = tmp;
        }
    }

    free(set->buckets);
    set->buckets = resized;
    return set;
}


/**
 * DESCRIPTION:
 * Inserts a object into a runtime set
 *
 * Buckets will be resized if:
 * size = # of buckets + # of buckets / 2
 * NOTE:
 * If a duplicate is found, then that node is replaced with our new info, no new Set Node is created.
 */
RtObject *rtset_insert(RtSet *set, RtObject *val)
{
    assert(set);
    assert(val);

    unsigned int index = GetIndexedHash(set, val);

    // if chain is empty
    if (!set->buckets[index])
    {
        set->buckets[index] = init_SetNode(val);
        set->size++;
        rtobj_refcount_increment1(val); 
        return val;
    }

    SetNode *ptr = set->buckets[index];

    while (ptr)
    {
        // replaces key/val pair in place
        if (rtobj_equal(ptr->obj, val))
        {
            // updates reference counts correctly
            rtobj_refcount_decrement1(ptr->obj); 
            ptr->obj = val;
            rtobj_refcount_increment1(val); 
            return val;
        }

        ptr = ptr->next;
    }

    SetNode *node = init_SetNode(val);
    node->next = set->buckets[index];
    set->buckets[index] = node;
    set->size++;

    // updates the reference count 
    rtobj_refcount_increment1(val);

    // Resizes buckets array if needed
    if (set->size == (set->bucket_size + set->bucket_size / 2))
    {
        if (!rtset_resize(set))
            MallocError();
    }

    return val;
}

/**
 * DESCRIPTION:
 * Gets element from set, if element is not found, then function returns NULL
 *
 * PARAMS:
 * set: set
 * obj: obj to get
 */
RtObject *rtset_get(const RtSet *set, const RtObject *obj)
{
    assert(set && obj);
    unsigned int index = GetIndexedHash(set, obj);
    if (!set->buckets[index])
        return NULL;

    SetNode *ptr = set->buckets[index];

    while (ptr)
    {
        // replaces key/val pair in place
        if (rtobj_equal(ptr->obj, obj))
            return ptr->obj;

        ptr = ptr->next;
    }

    return NULL;
}


/**
 * DESCRIPTION:
 * Removes an object from set, and returns the value in the map. 
 * This function will return NULL if the object was not found in the set
 *
 * This function will downsize the number of buckets if:
 * size == # of buckets / 2
 *
 */
RtObject *rtset_remove(RtSet *set, RtObject *obj)
{
    assert(set && obj);
    unsigned int index = GetIndexedHash(set, obj);

    if (!set->buckets[index])
        return NULL;

    SetNode *prev = NULL;
    SetNode *ptr = set->buckets[index];

    while (ptr)
    {
        // replaces key/val pair in place
        if (rtobj_equal(ptr->obj, obj))
        {
            if (!prev)
            {
                set->buckets[index] = ptr->next;
            }
            else
            {
                prev->next = ptr->next;
            }
            RtObject *tmp = ptr->obj;
            
            // updates reference count
            rtobj_refcount_decrement1(tmp);

            free(ptr);
            set->size--;

            // downsizes buckets array if needed
            if (set->size == set->bucket_size / 2 && set->bucket_size >= DOWNSIZING_THRESHOLD)
            {
                if (!rtset_downsize(set))
                    MallocError();
            }

            return tmp;
        }
        prev = ptr;
        ptr = ptr->next;
    }
    return NULL;
}


__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Gets all elements in the set and puts them into a NULL terminated array
 *
 * NOTE:
 * Returns NULL if malloc fails
 *
 * PARAMS:
 * set: set
 */
RtObject **
rtset_getrefs(const RtSet *set)
{
    assert(set);
    RtObject **arr = malloc(sizeof(RtObject *) * (set->size + 1));

    if (!arr)
    {
        MallocError();
        return NULL;
    }

    unsigned int index = 0;
    for (size_t i = 0; i < set->bucket_size; i++)
    {
        if (!set->buckets[i])
            continue;

        SetNode *ptr = set->buckets[i];
        while (ptr)
        {
            arr[index++] = ptr->obj;
            ptr = ptr->next;
        }
    }
    arr[set->size] = NULL;
    return arr;
}


/**
 * DESCRIPTION:
 * Frees runtime set, and its associated objects if desired
 *
 * PARAMS:
 * set: set to free
 * free_obj: wether the runtime objects in the set should also be freed
 * free_immutable: if any object is to be freed, then should immutable data also be freed
 * update_ref_counts: wether references should be updated
 */
void rtset_free(RtSet *set, bool free_obj, bool free_immutable, bool update_ref_counts)
{
    for (size_t i = 0; i < set->bucket_size; i++)
    {
        if (!set->buckets[i])
            continue;

        SetNode *ptr = set->buckets[i];
        while (ptr)
        {
            SetNode *tmp = ptr->next;
            RtObject *obj = ptr->obj;
            
            if(update_ref_counts)
                rtobj_refcount_decrement1(obj);

            free_SetNode(ptr, free_obj, free_immutable, update_ref_counts);


            ptr = tmp;
        }
    }
    free(set->buckets);
    free(set);
}


__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Creates a copy of a runtime set
 *
 * PARAMS:
 * set: set to copy
 * deepcpy: wether objects in set should be deep copied
 * add_to_GC: wether objects contained by set should be added to GC
 */
RtSet *
rtset_cpy(const RtSet *set, bool deepcpy, bool add_to_GC)
{
    assert(set);
    RtSet *cpy = init_RtSet(set->bucket_size);
    if (!cpy)
        return NULL;

    RtObject **refs = rtset_getrefs(set);

    for (int i = 0; refs[i] != NULL; i++) {
        RtObject *val = deepcpy ? rtobj_deep_cpy(refs[i], add_to_GC) : refs[i];

        // this function will update reference count
        rtset_insert(cpy, val);

        if(add_to_GC)
            add_to_GC_registry(val);
    }

    free(refs);
    assert(set->size == cpy->size);
    return cpy;
}


/**
 * DESCRIPTION:
 * Checks if 2 set are equivalent, i.e contains the same number of elements and all elements are equal
 *
 */
bool rtset_equal(const RtSet *set1, const RtSet *set2)
{
    assert(set1 && set2);
    if (set1 == set2)
        return true;
    if (set1->size != set2->size)
        return false;

    RtObject **refs = rtset_getrefs(set1);
    for (size_t i = 0; refs[i] != NULL; i++)
    {
        RtObject *tmp = rtset_get(set2, refs[i]);
        if (!tmp || !rtobj_equal(tmp, refs[i]))
        {
            free(refs);
            return false;
        }
    }

    free(refs);
    return true;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Function that clears a set of its elements
 * 
 * PARAMS:
 * set: set
 * free_obj: if rt objects should be freed
 * free_immutable: wether immutable rt object data should be free
 * update_ref_counts: wether reference counts should be updated
*/
RtSet *rtset_clear(RtSet *set, bool free_obj, bool free_immutable, bool update_ref_counts) {
    assert(set);

    for(size_t i=0; i < set->bucket_size; i++) {
        if(!set->buckets[i])
            continue;
        
        SetNode *node = set->buckets[i];
        while(node) {
            SetNode *tmp = node->next;

            // this function will update reference count correctly
            if(update_ref_counts)
                rtobj_refcount_decrement1(tmp->obj);

            free_SetNode(node, free_obj, free_immutable, update_ref_counts);
            set->size--;
            node = tmp;
        }

        set->buckets[i]= NULL;
    }

    assert(set->size == 0);
    return set;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Performs a union set operation on two sets, and returns a new set
 * 
 * PARAMS:
 * set1: set1
 * set2: set2
 * cpy: wether the contents of the new set should be run through rtobj_rt_preprocess first
 * add_to_GC: wether objs in the new set should be added to the GC registry
*/
RtSet *rtset_union(const RtSet *set1, const RtSet *set2, bool cpy, bool add_to_GC) {
    RtSet *newset = init_RtSet(DEFAULT_SET_BUCKETS);
    if(!newset) return NULL;
    RtObject **contents1 = rtset_getrefs(set1);
    RtObject **contents2 = rtset_getrefs(set2);

    // go through first set
    for(size_t i = 0; contents1[i] != NULL; i++) {
        RtObject *obj = contents1[i];
        if(cpy)
            obj = rtobj_rt_preprocess(contents1[i], false, add_to_GC);

        // this function will update the reference count
        rtset_insert(newset, obj);
    }

    // go through second set
    for(size_t i = 0; contents2[i] != NULL; i++) {
        RtObject *obj = contents2[i];

        if(cpy)
            obj = rtobj_rt_preprocess(contents2[i], false, add_to_GC);

        // this function will update the reference count
        rtset_insert(newset, obj);
    }

    free(contents1);
    free(contents2);
    return newset;
}

/**
 * DESCRIPTION:
 * Performs a intersection set operation on two sets, and returns a new set
 * 
 * PARAMS:
 * set1: set1
 * set2: set2
 * cpy: wether the contents of the new set should be shallow copied or passed directly by reference
 * add_to_GC: wether objs in the new set should be added to the GC registry
 * 
 * NOTE:
 * if a shallow copy is performed then object from the first set will be added to the new set
*/
RtSet *rtset_intersection(const RtSet *set1, const RtSet *set2, bool cpy, bool add_to_GC) {
    RtSet *newset = init_RtSet(DEFAULT_SET_BUCKETS);
    if(!newset) return NULL;

    RtObject **contents1 = rtset_getrefs(set1);
    RtObject **contents2 = rtset_getrefs(set2);

    for(size_t i = 0; contents1[i] != NULL && contents2[i] != NULL; i++) {
        RtObject *tmp1 = contents1[i];
        RtObject *tmp2 = contents2[i];
        
        if(rtset_get(set1, tmp2)) {
            RtObject *obj = tmp1;
            if(cpy)
                obj =  rtobj_rt_preprocess(obj, false, add_to_GC);

            // this function will update the reference count
            rtset_insert(newset, obj);
        }
    }

    free(contents1);
    free(contents2);
    return newset;
}


/**
 * DESCRIPTION:
 * Prints out set runtime object to standard output
*/
void rtset_print(const RtSet *set) {
    assert(set);

    size_t counter = 0;
    printf("{");
    for(size_t i=0; i < set->bucket_size; i++) {
        if(!set->buckets[i])
            continue;
        
        SetNode *node = set->buckets[i];
        while(node) {
            char *obj_to_str = rtobj_toString(node->obj);

            if (node->obj->type == STRING_TYPE)
                printf("\"%s\"", obj_to_str);
            else
                printf("%s", obj_to_str);

            free(obj_to_str);
            counter++;
            if(counter == set->size) 
                break;
            
            printf(", ");
            node = node->next;
        }
    }
    printf("}");
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Converts set to string
*/
char* rtset_toString(const RtSet *set) {
    assert(set);
    char buffer[100];
    buffer[0]='\0';
    snprintf(buffer, 100, "set@%p", set);
    char *strcpy = cpy_string(buffer);
    if(!strcpy) MallocError();
    return strcpy;
}