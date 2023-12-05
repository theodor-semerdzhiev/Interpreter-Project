#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/*
Generic HashMap implementation using chaining
*/
typedef struct ChainNode ChainNode;
typedef struct ChainNode
{
    void *data;
    void *key;
    ChainNode *next;
} ChainNode;

typedef struct ChainingList
{
    ChainNode *head;
    ChainNode *tail;
} ChainingList;

typedef struct GenericMap
{
    int size; // number of elements in map

    int max_buckets;
    ChainingList **buckets;

    unsigned int (*hash)(const void *);    // function to hash keys
    bool (*are_keys_equal)(const void *, const void *); // function to compare keys
    void (*free_key)(void *);        // function for freeing key
    void (*free_data) (void *)       // function to free data   

} GenericMap;

/* Mallocs Chaining list */
static ChainingList *malloc_chaining_list()
{
    ChainingList *chain = malloc(sizeof(ChainingList));
    chain->head = NULL;
    chain->tail = NULL;
    return chain;
}

/* Mallocs Chain Node */
static ChainNode *malloc_chain_node(void *key, void *data)
{
    ChainNode *node = malloc(sizeof(ChainNode));
    node->data = data;
    node->key = key;
    node->next = NULL;
    return node;
}

/* Helper for freeing chain node */
static void free_chain_node(GenericMap *map, ChainNode *node, bool free_key, bool free_data)
{
    if (!node)
        return;
    if(free_key)
        map->free_key(node->key);
    if(free_data)
        map->free_data(node->data);

    free(node);
}

/* Helper function that removes element from list, returns the data
return NULL if data was not found in list */
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

            if(free_key)
                map->free_key(node->key);

            free_chain_node(map, node, true, false);
            return data;
        }

        prev = node;
        node = node->next;
    }

    return NULL;
}

/* Resizes Buckets when map as become too large 
Returns the map itself 
*/
static GenericMap *resize_GenericMap(GenericMap *map) {
    int new_bucket_size = map->max_buckets * 2;
    ChainingList **resized_buckets = malloc(sizeof(ChainingList *) * new_bucket_size);

    if (!resized_buckets) {
        // Handle memory allocation failure
        return NULL;
    }

    for (int i = 0; i < new_bucket_size; i++) {
        resized_buckets[i] = NULL;
    }

    ChainingList **prev_buckets = map->buckets;
    int prev_bucket_size = map->max_buckets;

    map->buckets = resized_buckets;
    map->max_buckets = new_bucket_size;

    for (int i = 0; i < prev_bucket_size; i++) {
        if (!prev_buckets[i]) {
            free(prev_buckets[i]);
            continue;
        }

        ChainNode *head = prev_buckets[i]->head;

        while (head) {
            ChainNode *next = head->next;
            unsigned int new_index = map->hash(head->key) % new_bucket_size;

            if (!resized_buckets[new_index]) {
                resized_buckets[new_index] = malloc_chaining_list();
            }

            // ChainNode *new_node = malloc_chain_node(head->key, head->data);

            if (!resized_buckets[new_index]->head) {
                resized_buckets[new_index]->head = head;
                resized_buckets[new_index]->tail = head;
            } else {
                resized_buckets[new_index]->tail->next = head;
                resized_buckets[new_index]->tail = head;
            }

            // free(head);
            head = next;
        }
        free(prev_buckets[i]);
    }

    free(prev_buckets);
    return map;
}

#define DEFAULT_BUCKET_SIZE 100

/* Mallocs Generic Map
Params:
hash: Hash Function to be used to hash keys
compare: Function to compare keys with (return true if keys are equal)
free_key: Function to free keys
free_data: Function to free data mapped to keys
 */
GenericMap *init_GenericMap(
    unsigned int (*hash)(const void *), 
    bool (*compare_keys)(const void *, const void *), 
    void (*free_key)(void *),
    void (*free_data) (void *))
{   
    GenericMap *map = malloc(sizeof(GenericMap));
    // if allocation fails
    if(!map) return NULL;
    
    map->size = 0;
    map->max_buckets = DEFAULT_BUCKET_SIZE;
    map->buckets = malloc(sizeof(ChainingList *) * DEFAULT_BUCKET_SIZE);
    // if allocation fails
    if (!map->buckets) {
        free(map);
        return NULL;
    }

    map->hash = hash;
    map->are_keys_equal = compare_keys;
    map->free_key = free_key;
    map->free_data=free_data;

    for (int i = 0; i < map->max_buckets; i++)
    {
        map->buckets[i] = NULL;
    }

    return map;
}

/* Map contains key */
bool map_contains_key(const GenericMap *map, void *key)
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
/* If a none NULL value is returned, then value was mapped to key */
void* map_insert(GenericMap *map, void *key, void *value)
{
    unsigned int index = map->hash(key) % map->max_buckets;
    if (!map->buckets[index])
    {
        map->buckets[index] = malloc_chaining_list();
    }

    ChainNode *node = malloc_chain_node(key, value);
    ChainingList *list = map->buckets[index];

    ChainNode *head = list->head;
    while(head) {
        if(map->are_keys_equal(head->key, key)) {
            void *data = head->data;
            head->data=value;
            return data;
        }
        head=head->next;
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

    if(map->size == map->max_buckets * 2) {
        resize_GenericMap(map);
    }

    return NULL;
}
// TODO
/* Removes element from the map, return NULL if element is not in the map*/
void *map_remove_via_key(GenericMap *map, void *key, bool free_key)
{
    unsigned int index = map->hash(key) % map->max_buckets;

    if (!map->buckets[index])
    {
        return NULL;
    }
    ChainingList *list = map->buckets[index];
    void *data = remove_element_from_chain(map, key, list, free_key);

    if (!data)
    {
        return NULL;
    }

    map->size--;
    return data;
}

/* Removes all elements in map that return true on the filter function */
void map_filter_data(GenericMap *map, bool (*filter_data)(const void*),bool free_key, bool free_data) {
    for(int i=0; i < map->max_buckets; i++) {
        if(!map->buckets[i])
            continue;
        
        ChainNode *node = map->buckets[i]->head;

        while(node) {
            if(filter_data(node->data)) {
                ChainNode *tmp = node->next;
                void* data = remove_element_from_chain(map,node->key, map->buckets[i],free_key);
                
                if(free_data)
                    map->free_data(data);
                
                node=tmp;
            } else {
                node = node->next;
            }
        }
    }
} 

/* Frees Map */
void free_GenericMap(GenericMap *map, bool free_key, bool free_data)
{
    if(!map) return;
    
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
static void GenericMap_print_contents(const GenericMap *map) {

    printf("Size: %d \n Max Buckets: %d\n", map->size, map->max_buckets);
    for(int i=0; i < map->max_buckets; i++) {
        if(!map->buckets[i])
            continue;
        
        ChainNode* node = map->buckets[i]->head;

        while(node) {
            printf("(Key : %d, Value : %d, Test %d) -> ", *(int *)node->key, *(int*)node->data, map_contains_key(map, node->key));

            node=node->next;
        }
        printf("NULL\n");
    }

}

// for testing purposes (temporary)

static unsigned int hash(const int *i) {
    return *i;
}

static bool compare(const int *i1, const int *i2) {
    return (*i1) == (*i2);
}

static bool filter(const int *i1) {
    return (*i1) > 50;
}

int _main() {
    GenericMap *map = init_GenericMap(hash,compare,free, free);
    for(int i=0; i < 100; i++) {
        int *key = malloc(sizeof(int));
        *key = i;
        int *val = malloc(sizeof(int));
        *val = i ;
        map_insert(map, key,val);
    }

    for(int i=0; i < 100; i++) {
        if(i % 2 == 0) {
            printf("%d\n", map_contains_key(map, &i));
        } else {
            int k = i * 100;
            printf("%d\n", map_contains_key(map, &k));
        }
    }
    GenericMap_print_contents(map);

    map_filter_data(map, filter, false,true);
    

    GenericMap_print_contents(map);

    free_GenericMap(map, true, true);

}

