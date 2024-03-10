#include <assert.h>
#include "stkmachine.h"

/**
 * Below is the implementation of the stack machine used by the VM
 */

/**
 * DESCRIPTION:
 * Mallocs a stack machine node and arg allows you to set a default value for the stack machine
 */
static StkMachineNode *init_StkMachineNode(RtObject *obj, bool dispose)
{
    StkMachineNode *node = malloc(sizeof(StkMachineNode));
    if (!node)
        return NULL;
    node->next = NULL;
    node->obj = obj;
    node->dispose = dispose;
    return node;
}

/**
 * Logic for freeing Stack Machine Node, and object if its disposable,
 * and if dispose argument is set to true
 */
static void free_StackMachineNode(StkMachineNode *node, bool dispose)
{
    if (node->dispose && dispose) {
        rtobj_free(node->obj, false);
    }

    free(node);
}

/**
 * Mallocs a stack machine
 */
StackMachine *init_StackMachine()
{
    StackMachine *stk_machine = malloc(sizeof(StackMachine));
    if (!stk_machine)
        return NULL;
    stk_machine->head = NULL;
    stk_machine->size = 0;
    return stk_machine;
}

/**
 * Pops element from stack machine and returns Runtime Object
 * dispose: wether element should be freed, if its disposable
 * In which case it will return NULL
 */
RtObject *StackMachine_pop(StackMachine *stk_machine, bool dispose)
{
    assert(stk_machine);
    StkMachineNode *popped = stk_machine->head;
    RtObject *obj = popped->obj;
    stk_machine->head = popped->next;
    if (dispose)
    {
        free_StackMachineNode(popped, true);
    }
    else
    {
        free_StackMachineNode(popped, false);
    }
    stk_machine->size--;
    return dispose ? NULL : obj;
}

/**
 * Pushes RtObject to StackMachine
 */
RtObject *StackMachine_push(StackMachine *stk_machine, RtObject *obj, bool dispose)
{
    assert(stk_machine);
    assert(obj);
    StkMachineNode *node = init_StkMachineNode(obj, dispose);
    if (!node)
        return NULL;
    node->next = stk_machine->head;
    stk_machine->head = node;
    stk_machine->size++;

    return obj;
}

/**
 * DESCRIPTION:
 * Takes elements in the stack machine and creates a NULL terminated array
 */
RtObject **StackMachine_to_list(StackMachine *stk_machine)
{
    RtObject **arr = malloc(sizeof(RtObject *) * (stk_machine->size + 1));
    if (!arr)
        return NULL;
    unsigned int index = 0;
    StkMachineNode *node = stk_machine->head;
    while (node)
    {
        arr[index++] = node->obj;
        node = node->next;
    }
    arr[stk_machine->size] = NULL;
    return arr;
}

/**
 * DESCRIPTION:
 * Frees Stack Machine
 *
 * PARAMS:
 * stk_machine: stack machine
 * free_rtobj: wether objects still on the stack sould be freed
 */
void free_StackMachine(StackMachine *stk_machine, bool free_rtobj)
{
    if (!stk_machine)
        return;
    while (stk_machine->head)
    {
        StkMachineNode *tmp = stk_machine->head;
        stk_machine->head = tmp->next;

        if (free_rtobj)
            rtobj_free(tmp->obj, false);

        free(tmp);
    }

    free(stk_machine);
}