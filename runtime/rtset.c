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
            return val;
        }

        ptr = ptr->next;
    }

    SetNode *node = init_SetNode(val);
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

/**
 * DESCRIPTION:
 * Gets element from set, if element is not found, then function returns NULL
 *
 * PARAMS:
 * set: set
 * obj: obj to get
 */
RtObject *rtset_get(const RtSet *set, RtObject *obj)
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
 */
void rtset_free(RtSet *set, bool free_obj, bool free_immutable)
{
    for (size_t i = 0; i < set->bucket_size; i++)
    {
        if (!set->buckets[i])
            continue;

        SetNode *ptr = set->buckets[i];
        while (ptr)
        {
            SetNode *tmp = ptr->next;
            free_SetNode(ptr, free_obj, free_immutable);
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
 */
RtSet *
rtset_cpy(const RtSet *set, bool deepcpy)
{
    assert(set);
    RtSet *cpy = init_RtSet(set->bucket_size);
    if (!cpy)
        return NULL;

    RtObject **refs = rtset_getrefs(set);

    for (int i = 0; refs[i] != NULL; i++)
        rtset_insert(cpy, deepcpy ? rtobj_deep_cpy(refs[i]) : refs[i]);

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

/**
 * DESCRIPTION:
 * Function that clears a set of its elements
 * 
 * PARAMS:
 * set: set
 * free_obj: if rt objects should be freed
 * free_immutable: wether immutable rt object data should be freed
*/
RtSet *rtset_clear(RtSet *set, bool free_obj, bool free_immutable) {
    assert(set);

    for(size_t i=0; i < set->bucket_size; i++) {
        if(!set->buckets[i])
            continue;
        
        SetNode *node = set->buckets[i];
        while(node) {
            SetNode *tmp = node->next;
            free_SetNode(node, free_obj, free_immutable);
            set->size--;
            node = tmp;
        }

        set->buckets[i]= NULL;
    }

    assert(set->size == 0);
    return set;
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


__attribute__((warn_unused_result))
/**
 * ** DEPRECATED **
 * DESCRIPTION:
 * Converts set to human readable format
 *
 * PARAMS:
 * set: set
 */
char *
_rtset_toString(const RtSet *set)
{
    // handles empty string
    if (set->size == 0)
        return cpy_string("{}");

    RtObject **setobjs = rtset_getrefs(set);
    char *str = NULL;
    for (int i = 0; setobjs[i] != NULL; i++)
    {
        char *keystr = rtobj_toString(setobjs[i]);
        if (setobjs[i]->type == STRING_TYPE) {
            char *tmp = surround_string(keystr, setobjs[i]->data.String->length, '"', '"');
            free(keystr);
            keystr=tmp;
        }

        char *tmp = concat_strings(str, keystr);
        free(keystr);
        free(str);
        str = tmp;

        if (setobjs[i + 1] != NULL)
        {
            tmp = concat_strings(str, ", ");
            free(str);
            str = tmp;
        }
    }

    char *tmp = concat_strings("{", str);
    char *tmp_ = concat_strings(tmp, "}");
    free(str);
    free(tmp);
    str = tmp_;
    free(setobjs);
    return str;
}
