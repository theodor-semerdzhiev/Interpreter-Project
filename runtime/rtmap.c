#include <stdlib.h>
#include <assert.h>
#include "rtmap.h"
#include "rtobjects.h"
#include "../generics/utilities.h"

/**
 * DESCRIPTION:
 * This file contains the implementation of the runtime maps (i.e dicts in python basically)
 */

#define GetIndexedHash(map, key) rtobj_hash(key) % map->bucket_size;

typedef struct MapNode MapNode;
typedef struct MapNode
{
    RtObject *key;
    RtObject *value;
    MapNode *next;
} MapNode;

#define DEFAUL_RTMAP_BUCKET_SIZE 16

// Determines the minimal amount of buckets for resizing to be done
#define DOWNSIZING_THRESHOLD 64

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Initializes a new rtmap with a initial_bucket_size
 *
 * if the initial bucket size is smaller than the default, then the default value is used
 *
 * NOTE:
 * This function returns NULL, if malloc fails
 */
RtSet *
init_RtMap(unsigned long initial_bucket_size)
{
    RtSet *map = malloc(sizeof(RtSet));
    if (!map)
        return NULL;
    map->bucket_size =
        initial_bucket_size > DEFAUL_RTMAP_BUCKET_SIZE ? initial_bucket_size : DEFAUL_RTMAP_BUCKET_SIZE;

    map->buckets = calloc(map->bucket_size, sizeof(MapNode *));
    if (!map->buckets)
    {
        free(map);
        return NULL;
    }
    map->GCFlag = false;
    map->size = 0;
    return map;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Initializes MapNode with key and val
 */
static MapNode *
init_MapNode(RtObject *key, RtObject *val)
{
    MapNode *node = malloc(sizeof(MapNode));
    if (!node)
    {
        MallocError();
        return NULL;
    }
    node->key = key;
    node->value = val;
    node->next = NULL;
    return node;
}

/**
 * DESCRIPTION:
 * Helper for resizing the number of buckets by factor of 2 when the size of the map gets too big compared to the max buckets
 *
 * NOTE:
 * This function returns the modified input map, it will return NULL, if malloc fails
 * PARAMS:
 * map: map to be reisized
 * fact
 */
static RtSet *rtmap_resize(RtSet *map)
{
    assert(map);
    MapNode **resized = calloc(map->bucket_size * 2, sizeof(MapNode *));
    if (!resized)
        return NULL;

    size_t old_bucket_size = map->bucket_size;
    map->bucket_size *= 2;
    for (size_t i = 0; i < old_bucket_size; i++)
    {
        if (!map->buckets[i])
            continue;

        MapNode *ptr = map->buckets[i];
        while (ptr)
        {
            MapNode *tmp = ptr->next;
            unsigned int index = GetIndexedHash(map, ptr->key);
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

    free(map->buckets);
    map->buckets = resized;
    return map;
}

/**
 * DESCRIPTION:
 * Helper for downsizing the number of buckets when the size of the map is too small compared to the number of buckets
 *
 * NOTE:
 * This function returns the original map inputted
 *
 * PARAMS:
 * map: map to be downsized
 */
static RtSet *rtmap_downsize(RtSet *map)
{
    assert(map);
    MapNode **resized = calloc(map->bucket_size / 2, sizeof(MapNode *));
    if (!resized)
        return NULL;

    size_t old_bucket_size = map->bucket_size;
    map->bucket_size /= 2;
    for (size_t i = 0; i < old_bucket_size; i++)
    {
        if (!map->buckets[i])
            continue;

        MapNode *ptr = map->buckets[i];
        while (ptr)
        {
            MapNode *tmp = ptr->next;
            unsigned int index = GetIndexedHash(map, ptr->key);
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

    free(map->buckets);
    map->buckets = resized;
    return map;
}

/**
 * DESCRIPTION:
 * Frees Map node
 *
 * PARAMS:
 * node: Map Node to free
 * free_key: wether the associated key object should be freed
 * free_val: wether the associated value object should be freed
 */
static void free_MapNode(MapNode *node, bool free_key, bool free_val, bool free_immutable)
{
    if (!node)
        return;
    if (free_key)
        rtobj_free(node->key, free_immutable);

    if (free_val)
        rtobj_free(node->value, free_immutable);

    free(node);
}

/**
 * DESCRIPTION:
 * Inserts a key value pair inside given map
 *
 * Buckets will be resized if:
 * size = # of buckets + # of buckets / 2
 * NOTE:
 * If a duplicate is found, then that node is replaced with our new info, no new Map Node is created.
 */
RtObject *rtmap_insert(RtSet *map, RtObject *key, RtObject *val)
{
    assert(map && key && val);
    unsigned int index = GetIndexedHash(map, key);

    // if chain is empty
    if (!map->buckets[index])
    {
        map->buckets[index] = init_MapNode(key, val);
        map->size++;
        return val;
    }

    MapNode *ptr = map->buckets[index];

    while (ptr)
    {
        // replaces key/val pair in place
        if (rtobj_equal(ptr->key, key))
        {
            ptr->key = key;
            ptr->value = val;
            map->size++;
            return key;
        }

        ptr = ptr->next;
    }

    MapNode *node = init_MapNode(key, val);
    node->next = map->buckets[index];
    map->buckets[index] = node;
    map->size++;

    // Resizes buckets array if needed
    if (map->size == (map->bucket_size + map->bucket_size / 2))
    {
        if (!rtmap_resize(map))
            MallocError();
    }

    return val;
}

/**
 * DESCRIPTION:
 * Removes a key value pair from map, and returns the value in the map. This function will return NULL if the key was not found in the map
 *
 * This function will downsize the number of buckets if:
 * size == # of buckets / 2
 * NOTE:
 * Key inside the map is not freed, its assumed the GC will take care of that
 *
 */
RtObject *rtmap_remove(RtSet *map, RtObject *key)
{
    assert(map && key);
    unsigned int index = GetIndexedHash(map, key);

    if (!map->buckets[index])
        return NULL;

    MapNode *prev = NULL;
    MapNode *ptr = map->buckets[index];

    while (ptr)
    {
        // replaces key/val pair in place
        if (rtobj_equal(ptr->key, key))
        {
            if (!prev)
            {
                map->buckets[index] = ptr->next;
            }
            else
            {
                prev->next = ptr->next;
            }
            RtObject *tmp = ptr->key;
            free(ptr);
            map->size--;

            // downsizes buckets array if needed
            if (map->size == map->bucket_size / 2 && map->bucket_size >= DOWNSIZING_THRESHOLD)
            {
                if (!rtmap_downsize(map))
                    MallocError();
            }
            return tmp;
        }
        prev = ptr;
        ptr = ptr->next;
    }
    return NULL;
}

/**
 * DESCRIPTION:
 * Gets element mapped to key, if element is not found, then function returns NULL
 *
 * PARAMS:
 * map: map
 * key: key
 */
RtObject *rtmap_get(const RtSet *map, RtObject *key)
{
    assert(map && key);
    unsigned int index = GetIndexedHash(map, key);
    if (!map->buckets[index])
        return NULL;

    MapNode *ptr = map->buckets[index];

    while (ptr)
    {
        // replaces key/val pair in place
        if (rtobj_equal(ptr->key, key))
        {
            return ptr->value;
        }

        ptr = ptr->next;
    }

    return NULL;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Gets all elements in the map and puts them into a NULL terminated array
 *
 * NOTE:
 * Returns NULL if malloc fails
 * Array is formatted in the following way: [key1, val1, key2, val2, ....]
 *
 * PARAMS:
 * map: map
 * getkeys: wether keys should be included in array
 * getVals: wether vals should be included in array
 */
RtObject **
rtmap_getrefs(const RtSet *map, bool getkeys, bool getvals)
{
    assert(map);
    RtObject **arr = NULL;
    if (getkeys && getvals)
    {
        arr = malloc(sizeof(RtObject *) * (map->size * 2 + 1));
    }
    else
    {
        arr = malloc(sizeof(RtObject *) * (map->size + 1));
    }

    if (!arr)
    {
        MallocError();
        return NULL;
    }

    unsigned int index = 0;

    for (size_t i = 0; i < map->bucket_size; i++)
    {
        if (!map->buckets[i])
            continue;

        MapNode *ptr = map->buckets[i];
        while (ptr)
        {
            if (getkeys)
            {
                arr[index] = ptr->key;
                index++;
            }

            if (getvals)
            {
                arr[index] = ptr->value;
                index++;
            }

            ptr = ptr->next;
        }
    }
    arr[getkeys && getvals ? map->size * 2 : map->size] = NULL;
    return arr;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Creates a copy of a runtime map
 *
 * PARAMS:
 * map: map to copy
 * deepcpy_key: wether key object should be deep copied
 * deepcpy_val: wether value object should be deep copied
 */
RtSet *
rtmap_cpy(const RtSet *map, bool deepcpy_key, bool deepcpy_val)
{
    assert(map);
    RtSet *cpy = init_RtMap(map->bucket_size);
    if (!cpy)
        return NULL;

    RtObject **refs = rtmap_getrefs(map, true, true);

    for (int i = 0; refs[i] != NULL;)
    {
        rtmap_insert(
            cpy,
            deepcpy_key ? rtobj_deep_cpy(refs[i]) : refs[i],
            deepcpy_val ? rtobj_deep_cpy(refs[i + 1]) : refs[i + 1]);

        i += 2;
    }
    free(refs);
    assert(map->size == cpy->size);
    return cpy;
}

/**
 * DESCRIPTION:
 * Frees map, and its associated objects if desired
 *
 * PARAMS:
 * map: map to free
 * free_keys: wether the keys should be freed
 * free_vals: wether the values shouls be freed
 * free_immutable: if any object is to be freed, then should immutable data be freed as well
 */
void rtmap_free(RtSet *map, bool free_keys, bool free_vals, bool free_immutable)
{

    for (size_t i = 0; i < map->bucket_size; i++)
    {
        if (!map->buckets[i])
            continue;

        MapNode *ptr = map->buckets[i];
        while (ptr)
        {
            MapNode *tmp = ptr->next;
            free_MapNode(ptr, free_keys, free_vals, free_immutable);
            ptr = tmp;
        }
    }
    free(map->buckets);
    free(map);
}

/**
 * DESCRIPTION:
 * Simple helper for surrounding string by quotation marks
 */
static char *add_quotation_marks(char *str)
{
    char *tmp = concat_strings("\"", str);
    if (!tmp)
        MallocError();
    char *tmp1 = concat_strings(tmp, "\"");
    if (!tmp1)
        MallocError();
    free(str);
    free(tmp);
    return tmp1;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Converts map to human readable format
 *
 * PARAMS:
 * map: map
 */
char *
rtmap_toString(const RtSet *map)
{
    // handles empty string
    if (map->size == 0)
    {
        return cpy_string("{}");
    }

    RtObject **keyvals = rtmap_getrefs(map, true, true);
    char *str = NULL;
    for (int i = 0; keyvals[i] != NULL;)
    {
        char *keystr = rtobj_toString(keyvals[i]);
        if (keyvals[i]->type == STRING_TYPE)
            keystr = add_quotation_marks(keystr);

        char *valstr = rtobj_toString(keyvals[i + 1]);
        if (keyvals[i + 1]->type == STRING_TYPE)
            valstr = add_quotation_marks(valstr);

        char *tmp = concat_strings(keystr, " : ");
        char *tmp_ = concat_strings(tmp, valstr);
        free(tmp);
        tmp = concat_strings(str, tmp_);
        free(keystr);
        free(valstr);
        free(tmp_);
        free(str);
        str = tmp;

        if (keyvals[i + 2] != NULL)
        {
            tmp = concat_strings(str, ", ");
            free(str);
            str = tmp;
        }

        i += 2;
    }

    char *tmp = concat_strings("{", str);
    char *tmp_ = concat_strings(tmp, "}");
    free(str);
    free(tmp);
    str = tmp_;
    free(keyvals);
    return str;
}

/**
 * DESCRIPTION:
 * Checks if 2 maps are equivalent, i.e contains the same number of elements and each key-value pair is equivalent
 *
 */
bool rtmap_equal(const RtSet *map1, const RtSet *map2)
{
    assert(map1 && map2);
    if (map1 == map2)
        return true;
    if (map1->size != map2->size)
        return false;

    RtObject **refs = rtmap_getrefs(map1, true, false);
    for (size_t i = 0; refs[i] != NULL; i++)
    {
        RtObject *tmp2 = rtmap_get(map1, refs[i]);
        RtObject *tmp1 = rtmap_get(map1, refs[i]);

        if (!tmp1 || !tmp2 || !rtobj_equal(tmp2, tmp2))
        {
            free(refs);
            return false;
        }
    }
    free(refs);
    return true;
}
