#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include "memtracker.h"

/*
DESCRITPION:
This file contains the implementation of a memory tracker data structure.
Its especially useful for keeping track of malloced memory and freeing it all 
even if some pointers have gotten out of reach.

Its heavily used in the parsing frontend, by freeing all the memory that was recursively allocated in case of a syntax error.
*/

__attribute__((warn_unused_result))
/* Mallocs static memtracker pointer*/
MemoryTracker *init_memtracker()
{
    MemoryTracker *memtracker = (MemoryTracker *)malloc(sizeof(MemoryTracker));
    if(!memtracker) return NULL;
    memtracker->head = NULL;
    memtracker->tail = NULL;
    memtracker->size = 0;
    return memtracker;
}

__attribute__((warn_unused_result))
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

/**
 * DESCRIPTION:
 * Goes through memory tracker and removes the node matching the pointer, if its found
 * 
 * PARAM:
 * memtracker: instance of memory tracker
 * ptr: ptr to match with
 * free_ptr: wether pointer should be freed
*/
void remove_ptr_from_memtracker(MemoryTracker *memtracker, void *ptr, bool free_ptr) {
    if(!memtracker) return;

    MallocNode *head = memtracker->head;
    MallocNode *prev= NULL;
    while(head) {
        if(head->ptr == ptr) {
            if(free_ptr)
                head->free(ptr);
            
            if(prev) {
                prev->next=head->next;

            // if its the head we must remove
            } else {
                memtracker->head=head->next;
                if(!head->next)
                    memtracker->tail=NULL;
            }
            free(head);
            return;
        }
        prev = head;
        head = head->next;
    }
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