#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "memstack.h"

/* This file contains the implementation of stack data structure used across the entire project
to keep track of malloced memory and freed if n */
static struct memstack *stack = NULL;

/* Mallocs static memstack pointer*/
struct memstack *init_memstack()
{
    stack = (struct memstack *)malloc(sizeof(struct memstack));
    stack->bottom = NULL;
    stack->top = NULL;
    stack->size = 0;
    return stack;
}

/* Mallocs pointer (stack node) struct */
struct stacknode *malloc_pointer_struct()
{
    struct stacknode *pointer = malloc(sizeof(struct stacknode));
    pointer->above = NULL;
    pointer->below = NULL;
    pointer->free = NULL;
    pointer->ptr = NULL;
    return pointer;
}

/* Pushes stack node to memory stack */
void push_to_memstack(void *ptr, void (*free_func)(void *))
{
    if (!stack)
        init_memstack();

    struct stacknode *pointer = malloc_pointer_struct();
    pointer->ptr = ptr;
    pointer->free = free_func;

    if (!stack->top)
    {
        stack->top = pointer;
        stack->bottom = pointer;
    }
    else
    {
        stack->top->above = pointer;
        pointer->below = stack->top;
        stack->top = pointer;
    }
    stack->size++;
}

/* Clears the stack and frees all the pointers and their corresponding stacknodes */
void clear_memstack_ptrs()
{
    if (!stack)
        init_memstack();

    struct stacknode *pointer = stack->bottom;
    while (pointer)
    {
        struct stacknode *tmp = pointer->above;
        pointer->free(pointer->ptr);
        free(pointer);
        pointer = tmp;
    }

    stack->size = 0;
    stack->bottom = NULL;
    stack->top = NULL;
}

/* Empties the stack, but DOES NOT free the pointers */
void clear_memstack_without_ptrs()
{
    if (!stack)
        init_memstack();

    struct stacknode *pointer = stack->bottom;
    while (pointer)
    {
        struct stacknode *tmp = pointer->above;
        free(pointer);
        pointer = tmp;
    }

    stack->size = 0;
    stack->bottom = NULL;
    stack->top = NULL;
}

/*
Free's specific pointer and its corresponding stack node
Starting the search from top to bottom
*/
void free_memstack_ptr_top_bottom(void *ptr)
{
    if (!stack)
        init_memstack();

    struct stacknode *tmp = stack->top;
    while (tmp && tmp->ptr != ptr)
        tmp = tmp->below;

    if (!tmp)
        return;

    tmp->free(ptr);

    if (tmp->below)
        tmp->below->above = tmp->above;
    if (tmp->above)
        tmp->above->below = tmp->below;

    free(tmp);
    stack->size--;
}

/*
Free's specific pointer and its corresponding stack node
Starting the search from bottom to top
*/
void free_memstack_ptr_bottom_top(void *ptr)
{
    if (!stack)
        init_memstack();

    struct stacknode *tmp = stack->bottom;
    while (tmp && tmp->ptr != ptr)
        tmp = tmp->above;

    if (!tmp)
        return;

    tmp->free(ptr);

    if (tmp->below)
        tmp->below->above = tmp->above;
    if (tmp->above)
        tmp->above->below = tmp->below;

    free(tmp);
    stack->size--;
}

/* Free's the entire stack, including the stack pointer itself */
void free_memstack()
{
    clear_memstack_ptrs();
    free(stack);
    stack = NULL;
}

/* Free's the entire stack, including the stack pointer itself */
void free_memstack_without_ptr()
{
    clear_memstack_without_ptrs();
    free(stack);
    stack = NULL;
}