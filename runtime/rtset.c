#include <assert.h>
#include "../generics/utilities.h"
#include "rtobjects.h"
#include "rtset.h"

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
 * Initializes set
 */
RtSet *init_RtSet(size_t max_buckets)
{
    RtSet *set = malloc(sizeof(RtSet));
    if (!set)
        return NULL;

    set->bucket_size = max_buckets > DEFAULT_SET_BUCKETS ? max_buckets : DEFAULT_SET_BUCKETS;
    set->buckets = calloc(set->bucket_size, sizeof(MapNode *));
    if (!set->buckets)
    {
        free(set);
        return NULL;
    }

    set->size = 0;
    set->GCFlag = false;

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
 * Frees Set node
 *
 * PARAMS:
 * node: Map Node to free
 * free_val: wether the associated object should be freed
 */
static void free_SetNode(SetNode *node, bool free_val, bool free_immutable)
{
    if (!node)
        return;

    if (free_val)
        rtobj_free(node->obj, free_immutable);

    free(node);
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
        return val;
    }

    SetNode *ptr = set->buckets[index];

    while (ptr)
    {
        // replaces key/val pair in place
        if (rtobj_equal(ptr->obj, val))
        {
            ptr->obj = val;
            set->size++;
            return val;
        }

        ptr = ptr->next;
    }

    SetNode *node = init_MapNode(val);
    node->next = set->buckets[index];
    set->buckets[index] = node;
    set->size++;

    // Resizes buckets array if needed
    if (set->size == (set->bucket_size + set->bucket_size / 2))
    {
        if (!rtset_resize(set))
            MallocError();
    }

    return val;
}

