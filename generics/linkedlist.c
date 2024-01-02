#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "utilities.h"
#include "linkedlist.h"

/**
 * DESCRIPTION: This file contains the Generic Linked List Implementation
*/

/* Forward declaration */
typedef struct LLNode LLNode;

/**
 * DESCRIPTION:
 * Defines node of Linked List
*/
typedef struct LLNode
{
    void *data; // data pointer
    LLNode *next; // next node in list
    LLNode *prev; // previous node in list
} LLNode;



/**
 * DESCRIPTION: Helper for allocating memory for linked list node 
 * NOTE: Returns NULL if malloc error occurs
 * */
__attribute__((warn_unused_result))
static LLNode *malloc_llnode(void *data)
{
    LLNode *node = malloc(sizeof(LLNode));
    if (!node)
        return NULL;
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

/**
 * DESCRIPTION:
 * Initializes Generic Linked List
 * 
 * PARAMS:
 * is_data_equal: equality function
 * free_data: free function for data pointers
 * 
 * NOTE:
 * Returns NULL malloc error occurs
*/
__attribute__((warn_unused_result))
GenericLList *init_GenericLList(bool (*is_data_equal)(void *, void *), void (*free_data)(void *))
{
    GenericLList *list = malloc(sizeof(GenericLList));
    if (!list)
    {
        return NULL;
    }
    list->is_data_equal = is_data_equal;
    list->free_data = free_data;
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    return list;
}

/**
 * DESCRIPTION:
 * Checks wether Linked List contains data pointer using 'is_equal' function pointer stored by Linked List
 * 
 * PARAMS:
 * list: Linked List to query
 * data: data pointer used to query Linked List
*/
bool GenericLList_contains(const GenericLList *list, void *data)
{
    assert(list);
    LLNode *node = list->head;

    while (node)
    {
        if (list->is_data_equal(node->data, data))
        {
            return true;
        }
        node = node->next;
    }

    return false;
}

/**
 * DESCRIPTION:
 * Adds Element to the head of the Linked List
 * Will always return the newly added data 
 * 
 * PARAMS:
 * list: list to add to
 * data: data pointer to be added
 * 
 * NOTE:
 * Returns NULL if and only if malloc returns NULL, given data pointer != NULL
*/
__attribute__((warn_unused_result))
void *GenericLList_addFirst(GenericLList *list, void *data)
{
    assert(list);
    LLNode *node = malloc_llnode(data);
    if (!node)
    {
        return NULL;
    }
    node->next = list->head;
    if (!list->head)
    {
        list->tail = node;
    }
    list->head = node;
    list->length++;
    return data;
}

/**
 * DESCRIPTION:
 * Adds node to the end of the linked list
 * Should always return newly added data
 * 
 * PARAMS:
 * list: list to add to
 * data: data pointer to be added
 * 
 * NOTE:
 * returns NULL if and only if memory allocation fails, given data != NULL
 * */
__attribute__((warn_unused_result))
void *GenericLList_addLast(GenericLList *list, void *data)
{
    assert(list);
    LLNode *node = malloc_llnode(data);
    if (!node)
    {
        return NULL;
    }
    if (!list->head)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->length++;
    return data;
}

/**
 * DESCRIPTION:
 * Removes the last element of linked list, and returns the associated data pointer.
 * If input list is empty, then function will return NULL
 * 
 * PARAMS:
 * list: LinkedList to remove from
 * free_data: wether data should be freed, if set to true, this function will return always NULL
 * 
 * IMPORTANT:
 * If malloc returns NULL, then this function will also return NULL
 * Make sure you check if the input list is empty before hand before implementing error recovery logic
 * */
__attribute__((warn_unused_result))
void *GenericLList_popLast(GenericLList *list, bool free_data)
{
    assert(list);
    if (!list->tail)
        return NULL;

    LLNode *last = list->tail;
    void *data = last->data;

    if (!last->prev)
    {
        list->head = NULL;
        list->tail = NULL;
    }
    else
    {
        last->prev->next = NULL;
        list->tail = last->prev;
    }

    if (free_data)
        list->free_data(data);

    free(last);

    list->length--;
    return free_data ? NULL : data;
}

/**
 * DESCRIPTION:
 * Removes first element of Linked List, and returns the associated data pointer
 * if the list is empty, NULL will always be returned
 * 
 * PARAMS:
 * list: LinkedList to pop from
 * free_data: wether data should be freed, if set to true function will return NULL
 * 
 * IMPORTANT:
 * If malloc returns NULL, this function will return NULL
 * Make sure you check if the input list is empty before hand before implementing error recovery logic
 * */
__attribute__((warn_unused_result))
void *GenericLList_popFirst(GenericLList *list, bool free_data)
{
    assert(list);
    if (!list->head)
        return NULL;

    LLNode *first = list->head;
    void *data = first->data;

    // if list length == 1
    if (!first->next)
    {
        list->tail = first->next;
        list->head = first->next;
    }
    else
    {
        list->head = first->next;
        list->head->prev = NULL;
    }

    if (free_data)
        list->free_data(data);

    free(first);
    list->length--;

    return free_data ? NULL : data;
}


/**
 * DESCRIPTION:
 * Removes the first element that matches input data pointer using is_data_equal function pointer, 
 * The data pointer associated with the match is returned
 * if no match is found or list is empty, NULL is returned
 * 
 * PARAMS:
 * list: Generic Set to remove from
 * data: data pointer to perform matching
 * free_data: wether element that is found should be freed, NULL will always be returned if set to true
 */
__attribute__((warn_unused_result))
void *GenericLList_remove_matching_element(GenericLList *list, void *data, bool free_data)
{
    assert(list);

    LLNode *ptr = list->head;

    // if list is empty, then no match can be found
    if (!ptr)
    {
        return NULL;
    }

    while (ptr)
    {
        // checks if match is found
        if (list->is_data_equal(ptr->data, data))
        {
            // if node to be removed is the hea of list
            if (!ptr->prev)
            {
                list->head = ptr->next;

                if (ptr->next)
                    ptr->next->prev = NULL;

                if (!ptr->next)
                    list->tail = NULL;
            }
            else
            {
                ptr->prev->next = ptr->next;

                if (ptr->next)
                    ptr->next->prev = ptr->prev;

                if (!ptr->next)
                    list->tail = ptr->prev;
            }

            void *data = ptr->data;

            // frees data pointer if required
            if (free_data)
                list->free_data(data);

            free(ptr);
            return free_data ? NULL : data;
        }

        ptr = ptr->next;
    }

    return NULL;
}

/**
 * DESCRIPTION:
 * Takes all elements in the Linked List and aggregates them in a NULL terminated array
 * 
 * PARAMS:
 * list: Linked List to aggregate
 * 
 * NOTE:
 * Will return NULL if malloc fails
*/
__attribute__((warn_unused_result))
void **GenericLList_aggregate(GenericLList *list) {
    void **arr = malloc(sizeof(void*)*list->length+1);
    if(!arr)
        return NULL;
    int i=0;
    LLNode *ptr = list->head;
    while(ptr) {
        arr[i]=ptr->data;
        ptr=ptr->next;
        i++;
    }
    arr[list->length]=NULL;
    return arr;
}

/**
 * DESCRIPTION:
 * Gets the head of linked list (first element)
 * 
 * PARAMS:
 * list: The linked list
 * 
 * NOTE:
 * Returns NULL if list is empty
*/
__attribute__((warn_unused_result))
void *GenericLList_head(GenericLList *list) {
    if(!list->head) return NULL;
    return list->head->data;
}

/**
 * DESCRIPTION:
 * Gets the tail of linked list, (last element)
 * 
 * PARAMS:
 * list: The linked list
 * 
 * NOTE:
 * Returns NULL if list is empty
*/
__attribute__((warn_unused_result))
void *GenericLList_tail(GenericLList *list) {
    if(!list->head) return NULL;
    return list->tail->data;
}

/**
 * DESCRIPTION:
 * Frees the memory associated with the linked list
 * 
 * PARAMS:
 * list: list to be freed
 * free_data: wether data pointers should be freed aswell
 * 
 * NOTE:
 * if list pointer is NULL, function returns immediately 
 * */
void GenericLList_free(GenericLList *list, bool free_data)
{
    if (!list)
        return;

    LLNode *node = list->head;

    while (node)
    {
        LLNode *next = node->next;
        if (free_data)
            list->free_data(node->data);
        free(node);
        node = next;
    }

    free(list);
}
