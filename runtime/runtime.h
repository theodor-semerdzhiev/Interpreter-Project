#pragma once
#include "../compiler/compiler.h"
#include "../generics/hashmap.h"
#include "rtobjects.h"
#include "gc.h"

#define MAX_STACK_SIZE 5000

typedef struct IdentifierTable IdentTable;

typedef struct CallFrame {
    
    unsigned int pg_counter;
    ByteCodeList *pg;
    IdentTable *lookup;

    // the associated function, if applicable, 
    // if its global scope then it will be NULL
    RtFunction *function;

} CallFrame;

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

typedef struct RunTime {
    
    StackMachine *stk_machine;

    unsigned int stack_ptr;

} RunTime;

#define addToGCRegistry(obj) if(is_GC_Active()) add_to_GC_registry

IdentTable *init_IdentifierTable();
void Identifier_Table_add_var(IdentTable *table, const char *key, RtObject *obj, AccessModifier access);
RtObject *Identifier_Table_remove_var(IdentTable *table, const char *key);
RtObject *IdentifierTable_get(IdentTable *table, const char *key);
bool IdentifierTable_contains(IdentTable *table, const char *key);
int IdentifierTable_aggregate(IdentTable *table, const char* key);
RtObject **IdentifierTable_to_list(IdentTable *table);
Identifier **IdentifierTable_to_IdentList(IdentTable *table);
void free_IdentifierTable(IdentTable *table, bool free_rtobj);

StackMachine *init_StackMachine();
RtObject *StackMachine_pop(StackMachine *stk_machine, bool dispose);
RtObject *StackMachine_push(StackMachine *stk_machine, RtObject *obj, bool dispose);
RtObject **StackMachine_to_list(StackMachine *stk_machine);
void free_StackMachine(StackMachine *stk_machine, bool free_rtobj);

CallFrame *init_CallFrame(ByteCodeList *program, RtFunction *function);
void free_CallFrame(CallFrame *call, bool free_rtobj);

StackMachine *getCurrentStkMachineInstance();
int getCallStackPointer();
CallFrame **getCallStack();

RunTime *init_RunTime();
RtObject *lookup_variable(const char *var);
void free_RunTime(RunTime *rt);
void RunTime_push_callframe(CallFrame *frame);
CallFrame *RunTime_pop_callframe();

int prep_runtime_env(ByteCodeList *code);
CallFrame *perform_function_call(unsigned int arg_count);
int run_program();
bool isRuntimeActive();
void perform_cleanup();
