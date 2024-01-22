#pragma once

typedef struct LLNode LLNode;
/**
 * DESCRIPTION:
 * Defines Top level struct for Generic Linked List
*/
typedef struct GenericLList
{
    unsigned int length;     // Nb of elements list contains
    LLNode *head;   // tail of list
    LLNode *tail;   // head of list

    bool (*is_data_equal)(void *, void *);  // function to compare elements of the list (its void *data pointers)

    void (*free_data)(void *);              // function for freeing data pointers contains by the list

} GenericLList;

GenericLList *init_GenericLList(bool (*is_data_equal)(void *, void *), void (*free_data)(void *));
bool GenericLList_contains(const GenericLList *list, void *data);
void *GenericLList_addFirst(GenericLList *list, void *data);
void *GenericLList_addLast(GenericLList *list, void *data);
void *GenericLList_popLast(GenericLList *list, bool free_data);
void *GenericLList_popFirst(GenericLList *list, bool free_data);
void *GenericLList_remove_matching_element(GenericLList *list, void *data, bool free_data);
void **GenericLList_aggregate(GenericLList *list);
void *GenericLList_head(GenericLList *list);
void *GenericLList_tail(GenericLList *list);
void GenericLList_free(GenericLList *list, bool free_data);
