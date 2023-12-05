#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

/*
Generic Linked List Implementation
*/

typedef struct LLNode
{
    void *data;
    LLNode *next;
    LLNode *prev;
} LLNode;

typedef struct GenericLList
{
    int length;
    LLNode *head;
    LLNode *tail;

    bool (*is_data_equal)(void *, void *);

    void (*free_data)(void *);

} GenericLList;

/* Mallocs linked list node */
static LLNode *malloc_llnode(void *data)
{
    LLNode *node = malloc(sizeof(node));
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

/* Mallocs Linked List */
GenericLList *init_GenericLList(bool (*compare_data)(void *, void *), void (*free_data)(void *))
{
    GenericLList *list = malloc(sizeof(GenericLList));
    list->is_data_equal = compare_data;
    list->free_data = free_data;
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    return list;
}

/* Checks if list contains element using compare function */
bool LinkedList_contains(const GenericLList *list, void *data)
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

/* Adds data to head of linked list */
void LinkedList_addFirst(GenericLList *list, void *data)
{
    assert(list);
    LLNode *node = malloc_llnode(data);
    node->next = list->head;
    if (!list->head)
    {
        list->tail = node;
    }
    list->head = node;
    list->length++;
}

/* Adds node to end of linked list */
void LinkedList_addLast(GenericLList *list, void *data)
{
    assert(list);
    LLNode *node = malloc_llnode(data);
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
}

/* Removes last element of linked list */
void *LinkedList_popLast(GenericLList *list)
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

    free(data);
    list->length--;
    return data;
}

/* Removes first element of list */
void *LinkedList_popFirst(GenericLList *list)
{
    assert(list);
    if (!list->head)
        return NULL;

    LLNode *first = list->head;
    void *data = first->data;

    if (!first->next)
    {
        list->tail = first->prev;

        if (!first->prev)
        {
            list->head = NULL;
        }
        else
        {
            first->prev->next = NULL;
        }
    }
    else
    {
        list->head = first->next;
        list->head->prev = NULL;
    }

    free(first);
    list->length--;
    return data;
}
void *LinkedList_remove_matching_element(GenericLList *list, void *data)
{
    assert(list);

    LLNode *ptr = list->head;

    if (!ptr)
    {
        return NULL;
    }

    while (ptr)
    {
        if (list->is_data_equal(ptr->data, data))
        {
            if (!ptr->prev)
            {
                list->head = ptr->next;

                if (ptr->next)
                {
                    ptr->next->prev = NULL;
                }

                if (!ptr->next)
                {
                    list->tail = NULL;
                }
            }
            else
            {
                ptr->prev->next = ptr->next;

                if (ptr->next)
                {
                    ptr->next->prev = ptr->prev;
                }

                if (!ptr->next)
                {
                    list->tail = ptr->prev;
                }
            }

            void *removed_data = ptr->data;
            free(ptr);
            return removed_data;
        }

        ptr = ptr->next;
    }

    return NULL;
}

/* Frees the LinkedList */
void LinkedList_free(GenericLList *list)
{
    if (!list)
        return;

    LLNode *node = list->head;

    while (node)
    {
        LLNode *next = node->next;
        list->free_data(node->data);
        free(node);
        node = next;
    }

    free(list);
}
