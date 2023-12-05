#ifndef GENERIC_LINKEDLIST_H
#define GENERIC_LINKEDLIST_H

typedef struct GenericLList GenericLList;

GenericLList *init_GenericLList(bool (*compare_data)(void *, void *), void (*free_data)(void *));
bool LinkedList_contains(const GenericLList *list, void *data);
void LinkedList_addFirst(GenericLList *list, void *data);
void LinkedList_addLast(GenericLList *list, void *data);
void *LinkedList_popLast(GenericLList *list);
void *LinkedList_popFirst(GenericLList *list);
void *LinkedList_remove_matching_element(GenericLList *list, void *data);
void LinkedList_free(GenericLList *list);

#endif