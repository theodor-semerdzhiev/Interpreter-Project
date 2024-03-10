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