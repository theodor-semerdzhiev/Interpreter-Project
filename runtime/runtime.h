#pragma once
#include <setjmp.h>
#include "../compiler/compiler.h"
#include "../generics/hashmap.h"
#include "rtobjects.h"
#include "stkmachine.h"
#include "identtable.h"
#include "gc.h"

#define MAX_STACK_SIZE 16000

typedef struct CallFrame
{

    unsigned int pg_counter;
    ByteCodeList *pg;
    IdentTable *lookup;

    // the associated function, if applicable,
    // if its global scope then it will be NULL
    RtFunction *function;
    size_t line_number;       // line number where function was called
    char *code_file_location; // file where the code resides (i.e where the file where the functions is defined)

    // used by exception handling
    jmp_buf *exception_jump;
} CallFrame;

#define addToGCRegistry(obj) \
    if (is_GC_Active())      \
    add_to_GC_registry

// Useful macro in list, set, maps, and function args for adding objects to GC
// ONLY when needed (should be used with rtobj_rt_preprocess function)
#define addDisposablePrimitiveToGC(dispose, obj)                \
    if (dispose || (!dispose && rttype_isprimitive(obj->type))) \
        add_to_GC_registry(obj);

#define DisposableOrPrimitive(dispose, obj) (dispose || (!dispose && rttype_isprimitive(obj->type)))

StackMachine *init_StackMachine();
RtObject *StackMachine_pop(StackMachine *stk_machine, bool dispose);
RtObject *StackMachine_push(StackMachine *stk_machine, RtObject *obj, bool dispose);
RtObject **StackMachine_to_list(StackMachine *stk_machine);
void free_StackMachine(StackMachine *stk_machine, bool free_rtobj);

CallFrame *init_CallFrame(ByteCodeList *program, RtFunction *function, size_t line_number, const char *filename);

void free_CallFrame(CallFrame *call, bool free_rtobj);

StackMachine *getCurrentStkMachineInstance();
long getCallStackPointer();
CallFrame *getCurrentStackFrame();
CallFrame **getCallStack();

int init_RunTime();
RtObject *lookup_variable(const char *var);
void RunTime_push_callframe(CallFrame *frame);
CallFrame *RunTime_pop_callframe();

void dispose_disposable_obj(RtObject *obj, bool disposable);

int prep_runtime_env(ByteCodeList *code, const char *mainfile);
CallFrame *perform_function_call(size_t arg_count, size_t line_number);
int run_program();
bool isRuntimeActive();
void perform_cleanup();
