#pragma once
#include "rtobjects.h"

typedef struct StkMachineNode StkMachineNode;

typedef struct StkMachineNode
{
    RtObject *obj;
    StkMachineNode *next;
    size_t dispose; // wether object should be freed when popped
} StkMachineNode;

typedef struct StackMachine
{
    StkMachineNode *head;
    unsigned int size;
} StackMachine;

StackMachine *init_StackMachine();
RtObject *StackMachine_pop(StackMachine *stk_machine, bool dispose);
RtObject *StackMachine_push(StackMachine *stk_machine, RtObject *obj, bool dispose);
void free_StackMachine(StackMachine *stk_machine, bool free_rtobj, bool update_ref_counts);
