#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "hashset.h"
#include "utilities.h"

/* Generic HashSet Implementation using Chaining to handle hashing collisions */

/**
 * DESCRIPTION:
 * Defines node that stores data inside linked list (i.e Chaining list )
 */
typedef struct Node Node;
typedef struct Node
{
    void *data;
    Node *next;

} Node;

/**
 * DESCRIPTION: Defines Linked list used for chaining
 */
typedef struct ChainList
{
    Node *head;
    Node *tail;
    int length;
} ChainList;

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Helper function for allocating memory for Linked list (chain) node
 *
 * PARAMS:
 * data: initial value for the data stored by node
 *
 * NOTE: If malloc returns NULL, function return NULL
 *
 */
static Node *
_malloc_chain_node(void *data)
{
    Node *node = malloc(sizeof(Node));
    if (!node)
        return NULL;
    node->data = data;
    node->next = NULL;
    return node;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Helper function for allocating memory for Linked list (chain)
 *
 * NOTE: If malloc returns NULL, function returns NULL
 */
static ChainList *
_malloc_chain_list()
{
    ChainList *list = malloc(sizeof(ChainList));
    if (!list)
        return NULL;
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    return list;
}

/**
 * DESCRIPTION: Initial value for number of buckets set will contain
 */
#define DEFAULT_BUCKET_SIZE 32

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Initializes Generic Set
 *
 * PARAMS:
 * is_equal: function pointer to equality function
 * hash: hash function
 * free_data: function for freeing elements contained by set
 *
 * NOTE:
 * 1- is_equal function must return true if both pointers are considered to be equal, false otherwise
 * 2- If malloc error occurs, then function will return NULL
 */
GenericSet *
init_GenericSet(
    bool (*is_equal)(const void *, const void *),
    unsigned int (*hash)(const void *),
    void (*free_data)(void *))
{

    GenericSet *set = malloc(sizeof(GenericSet));
    // handles malloc error
    if (!set)
        return NULL;

    set->free_data = free_data;
    set->hash = hash;
    set->is_equal = is_equal;

    set->size = 0;

    set->max_buckets = DEFAULT_BUCKET_SIZE;
    set->buckets = malloc(sizeof(ChainList *) * DEFAULT_BUCKET_SIZE);
    // handles malloc error
    if (!set->buckets)
    {
        free(set);
        return NULL;
    }

    // Use memset to initialize the memory to zero
    memset(set->buckets, 0, sizeof(ChainList *) * DEFAULT_BUCKET_SIZE);

    return set;
}

/**
 * DESCRIPTION:
 * Returns wether set contains data
 *
 * PARAMS:
 * set: GenericSet to query
 * data: data used to query set
 */
bool GenericSet_get(const GenericSet *set, const void *data)
{
    unsigned int index = set->hash(data) % set->max_buckets;
    // if chain list was not created
    if (!set->buckets[index])
    {
        return false;
    }

    Node *node = set->buckets[index]->head;
    while (node)
    {
        if (set->is_equal(node->data, data))
        {
            return true;
        }
        node = node->next;
    }
    return false;
}

/**
 * DESCRIPTION:
 * Adds element to the set, should always return data argument that inputed,
 * Will only return NULL if malloc error occurs, make sure to handle this case properly
 *
 * PARAMS:
 * set: Generic Set to insert into
 * data: data to insert
 * free_duplicate_data: wether the data should be freed, if duplicate inside set is found
 *
 *
 * NOTE:
 * if duplicate value is found, then new data replaces old data
 * is_equal function pointer is used to determine duplicates
 *
 * IMPORTANT:
 * Make sure to free duplicate data properly since duplicates are not returned by this function
 * */
void *GenericSet_insert(GenericSet *set, void *data, bool free_duplicate_data)
{
    unsigned int index = set->hash(data) % set->max_buckets;

    // if chain list was not created
    if (!set->buckets[index])
    {
        set->buckets[index] = _malloc_chain_list();
        if (!set->buckets[index])
            return NULL;
    }

    Node *node = set->buckets[index]->head;
    // if list is empty
    if (!node)
    {
        Node *new_node = _malloc_chain_node(data);
        if (!new_node)
            return NULL;

        set->buckets[index]->head = new_node;
        set->buckets[index]->tail = new_node;
        set->size++;
        return data;
    }

    while (node)
    {
        // if element already in set, then function terminates
        if (set->is_equal(node->data, data))
        {
            if (free_duplicate_data)
                set->free_data(node->data);

            node->data = data;

            return data;
        }
        node = node->next;
    }

    // if element not in set, then we add it to the tail of the list
    Node *new_node = _malloc_chain_node(data);
    if (!new_node)
        return NULL;

    set->buckets[index]->tail->next = new_node;
    set->buckets[index]->tail = new_node;
    set->size++;
    return data;
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Removes element from set, returns the data itself
 * Return NULL if element is not found inside set
 *
 * PARAMS:
 * set: Set to remove from
 * data: data that should be removed
 *
 * NOTE:
 * 'is_equal' function pointer stored inside GenericSet struct is used to find element inside set
 */
void *
GenericSet_remove(GenericSet *set, void *data)
{
    unsigned int index = set->hash(data) % set->max_buckets;

    // if chain list was not created
    if (!set->buckets[index])
    {
        return NULL;
    }

    Node *head = set->buckets[index]->head;
    Node *prev = head;

    // Case where must remove head of list
    if (set->is_equal(head->data, data))
    {
        set->buckets[index]->head = head->next;
        if (!head->next)
            set->buckets[index]->tail = NULL;

        set->size--;
        void *data = head->data;
        free(head);
        return data;
    }
    while (head)
    {

        if (set->is_equal(head->data, data))
        {

            prev->next = head->next;

            if (!head->next)
                set->buckets[index]->tail = prev;

            void *removed_data = head->data;
            free(head);
            set->size--;
            return removed_data;
        }

        prev = head;
        head = head->next;
    }

    return NULL;
}

/**
 * DESCRIPTION:
 * Frees all memory associated with the input set
 *
 * PARAMS:
 * set: GenericSet to be freed
 * free_data: Wether the elements stored by the set should be freed
 */
void GenericSet_free(GenericSet *set, bool free_data)
{
    if (!set)
        return;

    for (int i = 0; i < set->max_buckets; i++)
    {
        if (!set->buckets[i])
        {
            continue;
        }

        Node *ptr = set->buckets[i]->head;
        while (ptr)
        {
            Node *next = ptr->next;

            if (free_data)
                set->free_data(ptr->data);

            free(ptr);
            ptr = next;
        }

        free(set->buckets[i]);
    }

    free(set->buckets);
    free(set);
}

/**
 * DESCRIPTION:
 * Removes all elements from input set that return true on the input filter function
 *
 * PARAMS:
 * set: GenericSet to filter
 * filter: filter function
 * free_data: Whether the data should be freed when removed from set
 */
void GenericSet_filter(GenericSet *set, bool (*filter)(void *), bool free_data)
{
    for (int i = 0; i < set->max_buckets; i++)
    {
        if (!set->buckets[i])
            continue;

        Node *node = set->buckets[i]->head;
        Node *prev = NULL;

        while (node)
        {
            if (filter(node->data))
            {
                if (free_data)
                {
                    set->free_data(node->data);
                }

                // if its first element
                if (!prev)
                {
                    set->buckets[i]->head = node->next;
                    if (!node->next)
                        set->buckets[i]->tail = NULL;

                    free(node);
                    node = set->buckets[i]->head;
                    continue;
                }
                else
                {
                    prev->next = node->next;
                    if (!node->next)
                        set->buckets[i]->tail = prev;

                    Node *next = node->next;
                    free(node);
                    node = next;
                    continue;
                }

                set->size--;
            }

            prev = node;
            node = node->next;
        }
    }
}

__attribute__((warn_unused_result))
/**
 * DESCRIPTION:
 * Aggregates all the elements of a set into a NULL terminated list
 *
 * PARAMS:
 * set: Generic Set to aggregate
 *
 * NOTE:
 * returns NULL, if malloc return NULL
 * */
void **
GenericSet_to_list(const GenericSet *set)
{
    void **list = malloc(sizeof(void *) * (set->size + 1));
    if (!list)
        return NULL;
    list[set->size] = NULL;
    int list_length = 0;
    for (int i = 0; i < set->max_buckets; i++)
    {
        if (!set->buckets[i])
            continue;

        Node *ptr = set->buckets[i]->head;
        while (ptr)
        {
            list[list_length] = ptr->data;
            ptr = ptr->next;
            list_length++;
        }
    }

    return list;
}

/* Used for debugging purposes */
void GenericSet_print_contents(const GenericSet *map, void (*print_data)(const void *))
{

    printf("Size: %d \n Max Buckets: %d\n", map->size, map->max_buckets);
    for (int i = 0; i < map->max_buckets; i++)
    {
        if (map->buckets[i])
        {
            Node *node = map->buckets[i]->head;

            while (node)
            {
                print_data(node->data);

                node = node->next;
            }
        }
        printf("NULL\n");
    }
}
