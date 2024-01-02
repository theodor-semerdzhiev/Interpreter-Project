#pragma once
#include "../compiler/compiler.h"
#include "../generics/hashmap.h"
#include "rtobjects.h"

#define MAX_STACK_SIZE 5000

typedef struct IdentifierTable IdentTable;

typedef struct CallFrame {
    
    int pg_counter;
    ByteCodeList *pg;
    IdentTable *lookup;

    // the associated function, if applicable, 
    // if its global scope then it will be NULL
    RtObject *function;

} CallFrame;

typedef struct StkMachineNode StkMachineNode;
typedef struct StackMachine StackMachine;

typedef struct RunTime {
    
    StackMachine *stk_machine;

    int stack_ptr;

} RunTime;

IdentTable *init_IdentifierTable();
void Identifier_Table_add_var(IdentTable *table, const char *key, RtObject *obj);
RtObject *Identifier_Table_remove_var(IdentTable *table, const char *key);
RtObject *IdentifierTable_get(IdentTable *table, const char *key);
bool IdentifierTable_contains(IdentTable *table, const char *key);
int IdentifierTable_aggregate(IdentTable *table, const char* key);
void free_IdentifierTable(IdentTable *table, bool free_rtobj);

StackMachine *init_StackMachine();
RtObject *StackMachine_pop(StackMachine *stk_machine, bool dispose);
RtObject *StackMachine_push(StackMachine *stk_machine, RtObject *obj, bool dispose);
void free_StackMachine(StackMachine *stk_machine, bool free_rtobj);

CallFrame *init_CallFrame(ByteCodeList *program, RtObject *function);
void free_CallFrame(CallFrame *call, bool free_rtobj);

RunTime *init_RunTime();
RtObject *lookup_variable(const char *var);
void free_RunTime(RunTime *rt);
void RunTime_push_callframe(CallFrame *frame);
CallFrame *RunTime_pop_callframe();

int prep_runtime_env(ByteCodeList *code);
CallFrame *perform_function_call(int arg_count);
int run_program();
bool isRuntimeActive();
void perform_cleanup();
