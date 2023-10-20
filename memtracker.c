#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "memtracker.h"

/* Mallocs static memtracker pointer*/
MemoryTracker *init_memtracker()
{
    MemoryTracker *memtracker = (MemoryTracker *)malloc(sizeof(MemoryTracker));
    memtracker->head = NULL;
    memtracker->tail = NULL;
    memtracker->size = 0;
    return memtracker;
}

/* Mallocs MallocNode struct */
static MallocNode *malloc_mallocnode_struct()
{
    MallocNode *pointer = malloc(sizeof(MallocNode));
    pointer->next = NULL;
    pointer->free = NULL;
    pointer->ptr = NULL;
    return pointer;
}

/* Pushes malloc node to memory tracker, returns the malloc node */
MallocNode *push_to_memtracker(MemoryTracker *memtracker, void *ptr, void (*free_func)(void *))
{
    MallocNode *mallocnode = malloc_mallocnode_struct();
    mallocnode->ptr = ptr;
    mallocnode->free = free_func;

    if(!memtracker->head) {
        memtracker->head=mallocnode;
        memtracker->tail=mallocnode;
    } else {
        memtracker->tail->next=mallocnode;
        memtracker->tail=memtracker->tail->next;
    }

    memtracker->size++;

    return mallocnode;
}

/* Frees all pointers in the memory tracker */
void clear_memtracker_pointers(MemoryTracker *memtracker)
{

    MallocNode *head = memtracker->head;
    while (head)
    {
        MallocNode *tmp = head->next;
        head->free(head->ptr);
        free(head);
        head = tmp;
    }

    memtracker->size = 0;
    memtracker->head = NULL;
    memtracker->tail = NULL;
}

/* Empties tracker, but DOES NOT free the pointers */
void clear_memtracker_without_pointers(MemoryTracker *memtracker)
{

    MallocNode *head = memtracker->head;
    while (head)
    {
        MallocNode *tmp = head->next;
        free(head);
        head = tmp;
    }

    memtracker->size = 0;
    memtracker->head = NULL;
    memtracker->tail = NULL;
}

/* Free's the entire stack, including the stack pointer itself */
void free_memtracker(MemoryTracker *memtracker)
{
    if(!memtracker) return;

    clear_memtracker_pointers(memtracker);
    free(memtracker);
}

/* Free's the entire stack, including the stack pointer itself */
void free_memtracker_without_pointers(MemoryTracker *memtracker)
{
    if(!memtracker) return;

    clear_memtracker_without_pointers(memtracker);
    free(memtracker);
}