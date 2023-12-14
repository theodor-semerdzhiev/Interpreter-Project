#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "hashset.h"

/* Generic HashSet Implementation using Chaining */

typedef struct Node Node;
typedef struct Node
{
    void *data;
    Node *next;

} Node;

typedef struct ChainList
{
    Node *head;
    Node *tail;
    int length;
} ChainList;


/* Mallocs chain Node */
static Node *_malloc_chain_node(void *data)
{
    Node *node = malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    return node;
}

/* Malloc Chain list */
static ChainList *_malloc_chain_list()
{
    ChainList *list = malloc(sizeof(ChainList));
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    return list;
}

/* Helper function for inserting node into Chain list */
static Node *_insert_into_chain_list(GenericSet *set, ChainList *list, void *data)
{

    if (!list->head)
    {
        Node *new_node = _malloc_chain_node(data);
        list->head = new_node;
        list->tail = new_node;
        list->length++;
        return new_node;
    }

    Node *node = list->head;

    while (node)
    {
        if (set->is_equal(data, node->data))
        {
            Node *new_node = _malloc_chain_node(data);
            new_node->next = node->next;
            node->next = new_node;
            list->length++;
            return new_node;
        }

        node = node->next;
    }

    Node *new_node = _malloc_chain_node(data);
    list->tail->next = new_node;
    list->tail = new_node;
    return new_node;
}

#define DEFAULT_BUCKET_SIZE 100

/* Mallocs Set struct, return NULL if allocation was not successful */
GenericSet *init_GenericSet(
    bool (*is_equal)(void *, void *),
    unsigned int (*hash)(void *),
    void (*free_data)(void *))
{

    GenericSet *set = malloc(sizeof(GenericSet));
    if (!set)
    {
        return NULL;
    }

    set->free_data = free_data;
    set->hash = hash;
    set->is_equal = is_equal;

    set->size = 0;

    set->max_buckets = DEFAULT_BUCKET_SIZE;
    set->buckets = malloc(sizeof(ChainList *) * DEFAULT_BUCKET_SIZE);
    if (!set->buckets)
    {
        free(set);
        return NULL;
    }

    for (int i = 0; i < DEFAULT_BUCKET_SIZE; i++)
    {
        set->buckets[i] = NULL;
    }
    return set;
}

/* Checks if element is contained within the set */
bool set_contains(const GenericSet *set, const void *data)
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

/* Adds element to the set, return element that was added */
void *set_insert(GenericSet *set, void *data)
{
    unsigned int index = set->hash(data) % set->max_buckets;

    // if chain list was not created
    if (!set->buckets[index])
    {
        set->buckets[index] = _malloc_chain_list();
    }

    Node *node = set->buckets[index]->head;
    // if list is empty
    if (!node)
    {
        Node *new_node = _malloc_chain_node(data);

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
            return data;
        }
        node = node->next;
    }

    // if element not in set, then we add it to the tail of the list
    Node *new_node = _malloc_chain_node(data);
    set->buckets[index]->tail->next = new_node;
    set->buckets[index]->tail = new_node;
    set->size++;
    return data;
}

/* Removes element from set, returns the element contained within the set
If element is not found, NULL is returned */
void *set_remove(GenericSet *set, void *data)
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

/* Frees memory for Generic set,
free_data: flag to indicate wether elements should be freed or not */
void free_GenericSet(GenericSet *set, bool free_data)
{
    if(!set) return;
    
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

/* Removes all elements where the filter function evals to true
filter: filter function predicate
free_data: flag to indicate whether data should be freed when removed
*/
void set_filter_remove(GenericSet *set, bool (*filter)(void *), bool free_data)
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
                    node=set->buckets[i]->head;
                    continue;
                }
                else
                {
                    prev->next = node->next;
                    if (!node->next)
                        set->buckets[i]->tail = prev;

                    free(node);
                    node = node->next;
                    continue;
                }

                set->size--;
            }

            prev = node;
            node = node->next;
        }
    }
}

/* Collects all the contents of the set and creates (mallocs) a list, same size as the set */
void** GenericSet_to_list(const GenericSet *set) {
    void** list = malloc(sizeof(void*) * (set->size+1));
    list[set->size]=NULL;
    int list_length = 0;
    for(int i=0; i < set->max_buckets; i++) {
        if(!set->buckets[i]) continue;
        
        Node *ptr = set->buckets[i]->head;
        while(ptr) {
            list[list_length]=ptr->data;
            ptr = ptr->next;
            list_length++;
        }
    }

    return list;
}




/* Used for debugging purposes */
void GenericSet_print_contents(const GenericSet *map, void (*print_data)(const void*)) {

    printf("Size: %d \n Max Buckets: %d\n", map->size, map->max_buckets);
    for(int i=0; i < map->max_buckets; i++) {
        if(map->buckets[i]) { 
            Node* node = map->buckets[i]->head;

            while(node) {
                print_data(node->data);

                node=node->next;
            }
        }
        printf("NULL\n");
    }

}


/* For testing purposes */
//////////////////////////
static bool integers_equal(const int *integer1, const int *integer2) {
    if (integer1 == NULL || integer2 == NULL) return false;
    return (*integer1) == (*integer2);
}

/* Hash function for integers */
static unsigned int hash_int(const int *integer) {
    // assert(integer);
    // const unsigned int A = 2654435769;  // A is a constant fraction close to (sqrt(5) - 1) / 2
    // return (unsigned int)(A * (*integer));
    return (*integer);
}

static bool filter(const int *integer) {
    return (*integer) < 50;
}

int _main() {
    GenericSet *set = init_GenericSet(integers_equal, hash_int, free);

    for(int i=0; i < 5000; i++) {
        int *num = malloc(sizeof(int));
        *num=i;
        set_insert(set, num);
    }

    // GenericSet_print_contents(set);
    set_filter_remove(set, filter, true);
    // GenericSet_print_contents(set);

    free_GenericSet(set, true);
    return 0;
}
