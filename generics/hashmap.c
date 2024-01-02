#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/*
Generic HashMap implementation using chaining to handle collisions 
*/

typedef struct ChainNode ChainNode;

/**
 * DESCRIPTION:
 * Defines a node in the ChaininList (i.e linked list)
*/
typedef struct ChainNode
{
    void *data;
    void *key;
    ChainNode *next;
} ChainNode;

/**
 * DESCRIPTION:
 * Defines a Linked list to contain key value pairs 
*/
typedef struct ChainingList
{
    ChainNode *head;
    ChainNode *tail;
} ChainingList;


/**
 * DESCRIPTION:
 * Defines a the top level struct for a Generic Map
 * Contains functions pointers to customize the behaviour of the data structure 
*/
typedef struct GenericMap
{
    int size; // number of elements in map

    int max_buckets;
    ChainingList **buckets;

    unsigned int (*hash)(const void *);                 // function to hash keys (hash function)
    bool (*are_keys_equal)(const void *, const void *); // function to compare keys
    void (*free_key)(void *);                           // function for freeing key
    void (*free_data)(void *);                          // function to free data

} GenericMap;

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Allocates memory for a Chaining List (i.e Linked List)
 * 
 * NOTE: Returns NULL if malloc returns NULL
*/
static ChainingList *malloc_chaining_list()
{
    ChainingList *chain = malloc(sizeof(ChainingList));
    if (!chain)
        return NULL;
    chain->head = NULL;
    chain->tail = NULL;
    return chain;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Allocates memory for a Chaining List node
 * 
 * PARAMS:
 * key: initial value for key field
 * data: initial value for data field
 * 
 * NOTE: Returns NULL if malloc returns NULL
 * 
*/
static ChainNode *malloc_chain_node(void *key, void *data)
{
    ChainNode *node = malloc(sizeof(ChainNode));
    if (!node)
        return NULL;
    node->data = data;
    node->key = key;
    node->next = NULL;
    return node;
}

/**
 * DESCRIPTION:
 * Helper for freeing chain node (i.e linkedlist node)
 * 
 * PARAMS:
 * 
 * map: map that contains node, needed for accessing function pointers
 * node: Chain node to free
 * free_key: if key should be freed
 * free_data: if data should be freed
 * 
 * NOTE: If node param is NULL, function returns
*/
static void free_chain_node(GenericMap *map, ChainNode *node, bool free_key, bool free_data)
{
    if (!node)
        return;
    if (free_key)
        map->free_key(node->key);
    if (free_data)
        map->free_data(node->data);

    free(node);
}

/**
 * DESCRIPTION:
 * Helper for properly removing element from chain (i.e linked list)
 * 
 * PARAMS:
 * map: map to remove from, needed for accessing function pointers
 * key: key to be removed
 * list: Linked list to search
 * free_key: wether key pointer should be freed if node to be removed is found
 * 
 * NOTE: Function returns NULL if list is NULL or list is not NULL but does not contains any elements
 * */
static void *remove_element_from_chain(GenericMap *map, void *key, ChainingList *list, bool free_key)
{

    if (!list || !list->head)
        return NULL;

    ChainNode *node = list->head;
    ChainNode *prev = NULL;

    while (node)
    {
        if (map->are_keys_equal(node->key, key))
        {
            if (!node->next)
            {
                list->tail = prev;
            }

            if (prev)
            {
                prev->next = node->next;
            }
            else
            {
                list->head = node->next;
            }

            void *data = node->data;

            if (free_key)
                map->free_key(node->key);

            free_chain_node(map, node, true, false);
            return data;
        }

        prev = node;
        node = node->next;
    }

    return NULL;
}

/**
 * DESCRIPTION:
 * Helper for resizing Buckets array, by factor of 2, when the number of elements in the map as become too large
 * Returns the map itself
 * 
 * Parameters:
 * map: HashMap to resize 
 * 
 * NOTE: returns NULL if memory error occurs
 */
static GenericMap *resize_GenericMap(GenericMap *map)
{
    int new_bucket_size = map->max_buckets * 2;
    ChainingList **resized_buckets = malloc(sizeof(ChainingList *) * new_bucket_size);

    if (!resized_buckets)
    {
        // Handle memory allocation failure
        return NULL;
    }

    for (int i = 0; i < new_bucket_size; i++)
    {
        resized_buckets[i] = NULL;
    }

    ChainingList **prev_buckets = map->buckets;
    int prev_bucket_size = map->max_buckets;

    map->buckets = resized_buckets;
    map->max_buckets = new_bucket_size;

    // reinserts elements in new buckets
    for (int i = 0; i < prev_bucket_size; i++)
    {
        if (!prev_buckets[i])
        {
            free(prev_buckets[i]);
            continue;
        }

        ChainNode *head = prev_buckets[i]->head;
        // inserts all elements in array
        while (head)
        {
            ChainNode *next = head->next;
            unsigned int new_index = map->hash(head->key) % new_bucket_size;

            if (!resized_buckets[new_index])
            {
                resized_buckets[new_index] = malloc_chaining_list();
                // Handles memory allocation error
                if (!resized_buckets[new_index]) {
                    free(resized_buckets);
                    return NULL;
                }
            }

            if (!resized_buckets[new_index]->head)
            {
                resized_buckets[new_index]->head = head;
                resized_buckets[new_index]->tail = head;
            }
            else
            {
                resized_buckets[new_index]->tail->next = head;
                resized_buckets[new_index]->tail = head;
            }

            head = next;
        }
        free(prev_buckets[i]);
    }

    free(prev_buckets);
    return map;
}

#define DEFAULT_BUCKET_SIZE 100

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Initializes Generic Map
 * 
 * PARAMS:
 * hash: Hash Function to be used to hash keys
 * compare: Function to compare keys (return true if keys are equal, false otherwise)
 * free_key: Function to free keys
 * free_data: Function to free data mapped to keys
 *
 * NOTE: Returns NULL if memory error occurs
 */
GenericMap *init_GenericMap(
    unsigned int (*hash)(const void *),
    bool (*compare_keys)(const void *, const void *),
    void (*free_key)(void *),
    void (*free_data)(void *))
{
    GenericMap *map = malloc(sizeof(GenericMap));
    // if allocation fails
    if (!map)
        return NULL;

    map->size = 0;
    map->max_buckets = DEFAULT_BUCKET_SIZE;
    map->buckets = malloc(sizeof(ChainingList *) * DEFAULT_BUCKET_SIZE);
    // if allocation fails
    if (!map->buckets)
    {
        free(map);
        return NULL;
    }

    map->hash = hash;
    map->are_keys_equal = compare_keys;
    map->free_key = free_key;
    map->free_data = free_data;

    memset(map->buckets, 0, sizeof(ChainingList *) * DEFAULT_BUCKET_SIZE);

    return map;
}

/**
 * DESCRIPTION:
 * Returns wether map contains a contains a key value pair matching the key
 * 
 * PARAMS:
 * map: map to query
 * key: key to query with
*/
bool GenericHashMap_contains_key(const GenericMap *map, void *key)
{
    unsigned int index = map->hash(key) % map->max_buckets;

    if (!map->buckets[index])
    {
        return false;
    }

    ChainNode *tmp = map->buckets[index]->head;

    while (tmp)
    {
        if (map->are_keys_equal(key, tmp->key))
            return true;

        tmp = tmp->next;
    }

    return false;
}
/**
 * DESCRIPTION:
 * Inserts a key value pair into map
 * If a duplicate mapping already exists then, its replaced by the new value
 * This function will always returns the new value (given as input)
 *
 * PARAMS:
 * map: HashMap to insert into to
 * key: key to map to
 * value: value that will be mapped to the key
 * free_duplicate_value: wether duplicate mapping value should be freed
 * 
 * NOTE: Returns NULL if memory error occurs, otherwise returns the new value
 * */
void *GenericHashMap_insert(GenericMap *map, void *key, void *value, bool free_duplicate_value)
{
    unsigned int index = map->hash(key) % map->max_buckets;
    if (!map->buckets[index])
    {
        map->buckets[index] = malloc_chaining_list();
        if (!map->buckets[index])
            return NULL;
    }

    ChainNode *node = malloc_chain_node(key, value);
    if (!node)
        return NULL;
    ChainingList *list = map->buckets[index];

    ChainNode *head = list->head;
    while (head)
    {
        if (map->are_keys_equal(head->key, key))
        {
            void *data = head->data;
            if (free_duplicate_value)
                map->free_data(data);

            head->data = value;
            return value;
        }
        head = head->next;
    }

    if (!list->head)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        list->tail = list->tail->next;
    }

    map->size++;

    if (map->size == map->max_buckets * 2)
    {
        resize_GenericMap(map);
    }

    return value;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Gets value mapped to key, return NULL if key value pair is not in map
 *
 * PARAMS:
 * map: Map to query from
 * key: key to query with
 */
void *GenericHashMap_get(GenericMap *map, const void *key)
{
    unsigned int index = map->hash(key) % map->max_buckets;
    // key value pair does not exist
    if (!map->buckets[index])
        return NULL;

    ChainNode *tmp = map->buckets[index]->head;
    while (tmp)
    {
        if (map->are_keys_equal(key, tmp->key))
            return tmp->data;

        tmp = tmp->next;
    }

    return NULL;
}

/**
 * DESCRIPTION:
 * Removes element from the map, returns the value, 
 * returns NULL if key value pair is not found in the map
 * 
 * PARAMS:
 * map: map to remove key from
 * key: key to remove
 * free_key: wether key pointer should be freed after removal
 * */
void *GenericHashmap_remove_key(GenericMap *map, void *key, bool free_key)
{
    unsigned int index = map->hash(key) % map->max_buckets;
    // key value pair does not exist
    if (!map->buckets[index])
        return NULL;

    ChainingList *list = map->buckets[index];
    void *data = remove_element_from_chain(map, key, list, free_key);

    // key value pair does not exist
    if (!data)
        return NULL;

    map->size--;
    return data;
}

/**
 * DESCRIPTION:
 * Removes ALL elements in Map that return true on input filter function
 * 
 * PARAMS:
 * map: Hashmap to be filtered
 * filter_data: pointer to filter function
 * free_key: wether key pointer should be freed if key value pair is to be removed 
 * free_data: wether data pointer sould be freed if key value pair is to be removed 
*/
void map_filter_data(GenericMap *map, bool (*filter_data)(const void *), bool free_key, bool free_data)
{
    for (int i = 0; i < map->max_buckets; i++)
    {
        if (!map->buckets[i])
            continue;

        ChainNode *node = map->buckets[i]->head;

        while (node)
        {
            if (filter_data(node->data))
            {
                ChainNode *tmp = node->next;
                void *data = remove_element_from_chain(map, node->key, map->buckets[i], free_key);

                if (free_data)
                    map->free_data(data);

                node = tmp;
            }
            else
            {
                node = node->next;
            }
        }
    }
}

/**
 * DESCRIPTION:
 * Frees all memory associated with Map
 * 
 * PARAMS:
 * map: Map to be freed
 * free_key: wether key pointers should be freed
 * free_data: wether data pointers should be freed
*/
void free_GenericMap(GenericMap *map, bool free_key, bool free_data)
{
    if (!map)
        return;

    for (int i = 0; i < map->max_buckets; i++)
    {
        if (!map->buckets[i])
            continue;

        ChainNode *ptr = map->buckets[i]->head;

        while (ptr)
        {
            ChainNode *tmp = ptr->next;
            free_chain_node(map, ptr, free_key, free_data);
            ptr = tmp;
        }

        free(map->buckets[i]);
    }
    free(map->buckets);
    free(map);
}

/* Used for debugging purposes */
static void GenericMap_print_contents(const GenericMap *map)
{

    printf("Size: %d \n Max Buckets: %d\n", map->size, map->max_buckets);
    for (int i = 0; i < map->max_buckets; i++)
    {
        if (!map->buckets[i])
            continue;

        ChainNode *node = map->buckets[i]->head;

        while (node)
        {
            printf("(Key : %d, Value : %d, Test %d) -> ", *(int *)node->key, *(int *)node->data, GenericHashMap_contains_key(map, node->key));

            node = node->next;
        }
        printf("NULL\n");
    }
}
