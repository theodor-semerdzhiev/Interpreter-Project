#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../generics/utilities.h"
#include "../compiler/compiler.h"
#include "builtins.h"
#include "runtime.h"
#include "rtlists.h"
#include "gc.h"

/**
 * DESCRIPTION:
 * This File contains the implementation of the runtime environment.
 * Includes:
 * - Main code loop
 * - Computation Stack
 *
 */

static RunTime *env;

/**
 * DESCRIPTION:
 * Below is the implementation for the variable lookup table
 */
typedef struct Identifier Identifier;
typedef struct Identifier
{
    char *key;
    RtObject *obj;
    Identifier *next;
    AccessModifier access;
} Identifier;

typedef struct IdentifierTable
{
    Identifier **buckets;
    int bucket_count;
    int size;
} IdentTable;

#define DEFAULT_IDENT_TABLE_BUCKET_SIZE 64

/**
 * Creates a new Identifer node for Identifier Table
 */
static Identifier *init_Identifier(const char *varname, RtObject *obj, AccessModifier access)
{
    Identifier *node = malloc(sizeof(Identifier));
    if (!node)
        return NULL;
    node->obj = obj;
    node->key = malloc_string_cpy(NULL, varname);
    node->access = access;
    node->next = NULL;
    return node;
}

/**
 * Frees identifier node
 * free_rtobj: wether Runtime Object should also be freed
 */
static void free_Identifier(Identifier *node, bool free_rtobj)
{
    if (!node)
        return;
    free(node->key);
    if (free_rtobj)
        rtobj_free(node->obj, false);

    free(node);
}

/**
 * Creates a new Identifier Table
 *
 */
IdentTable *init_IdentifierTable()
{
    IdentTable *table = malloc(sizeof(IdentTable));
    if (!table)
        return NULL;
    table->bucket_count = DEFAULT_IDENT_TABLE_BUCKET_SIZE;
    table->buckets = calloc(table->bucket_count, sizeof(Identifier *));
    if (!table->buckets)
    {
        free(table);
        return NULL;
    }
    table->size = 0;
    return table;
}

/**
 * Maps variable name to Runtime Object
 */
void Identifier_Table_add_var(IdentTable *table, const char *key, RtObject *obj, AccessModifier access)
{
    assert(table);
    unsigned int index = djb2_string_hash(key) % table->bucket_count;
    if (!table->buckets[index])
    {
        table->buckets[index] = init_Identifier(key, obj, access);
    }
    else
    {
        Identifier *node = init_Identifier(key, obj, access);
        node->next = table->buckets[index];
        table->buckets[index] = node;
    }
    table->size++;
}

/**
 * Removes variable from table, and returns the mapped RunTime object
 * If variable is not present, then function return NULL
 */
RtObject *Identifier_Table_remove_var(IdentTable *table, const char *key)
{
    assert(table);
    unsigned int index = djb2_string_hash(key) % table->bucket_count;

    Identifier *node = table->buckets[index];
    Identifier *prev = NULL;
    while (node)
    {
        if (strings_equal(node->key, key))
        {
            RtObject *obj = node->obj;
            if (!prev)
            {
                table->buckets[index] = node->next;
                free_Identifier(node, false);
                table->size--;
                return obj;
            }
            else
            {
                prev->next = node->next;
                free_Identifier(node, false);
                table->size--;
                return obj;
            }
        }

        prev = node;
        node = node->next;
    }

    return NULL;
}

/**
 * Fetches the object associated with the key, returns NULL if key does not exist
 */
RtObject *IdentifierTable_get(IdentTable *table, const char *key)
{
    unsigned int index = djb2_string_hash(key) % table->bucket_count;
    if (!table->buckets[index])
        return NULL;

    Identifier *node = table->buckets[index];
    while (node)
    {
        // if (strings_equal(node->key, "fib") && strings_equal(key, "fib"))
        // {
        //     printf("GET %d: %s\n", env->stack_ptr, obj_type_toString(node->obj));
        // }

        if (strings_equal(node->key, key))
            return node->obj;
        node = node->next;
    }
    return NULL;
}

/**
 * Checks if Identifier table contains key mapping
 */
bool IdentifierTable_contains(IdentTable *table, const char *key)
{
    unsigned int index = djb2_string_hash(key) % table->bucket_count;
    if (!table->buckets[index])
        return false;

    Identifier *node = table->buckets[index];
    while (node)
    {
        if (strings_equal(node->key, key))
            return true;
        node = node->next;
    }
    return false;
}

/**
 * DESCRIPTION:
 * Takes all elements of the table and puts them in a list (NULL terminated)
 *
 * NOTE:
 * Function returns NULL if malloc fails
 */
RtObject **IdentifierTable_to_list(IdentTable *table)
{
    RtObject **list = malloc(sizeof(RtObject *) * (table->size + 1));
    if (!list)
        return NULL;

    unsigned int count = 0;
    for (int i = 0; i < table->bucket_count; i++)
    {
        if (!table->buckets[i])
            continue;

        Identifier *ptr = table->buckets[i];
        while (ptr)
        {
            list[count++] = ptr->obj;
            ptr = ptr->next;
        }
    }
    list[table->size] = NULL;
    return list;
}

/**
 * DESCRIPTION:
 * Takes all Identifier nodes in the table puts them in a NULL terminated list
 */
Identifier **IdentifierTable_to_IdentList(IdentTable *table)
{
    assert(table);
    Identifier **list = malloc(sizeof(RtObject *) * (table->size + 1));
    if (!list)
        return NULL;

    unsigned int count = 0;
    for (int i = 0; i < table->bucket_count; i++)
    {
        if (!table->buckets[i])
            continue;

        Identifier *ptr = table->buckets[i];
        while (ptr)
        {
            list[count++] = ptr;
            ptr = ptr->next;
        }
    }

    list[table->size] = NULL;
    return list;
}

/**
 * Counts the number of Identifier Mappings contained within map
 */
int IdentifierTable_aggregate(IdentTable *table, const char *key)
{
    unsigned int index = djb2_string_hash(key) % table->bucket_count;
    if (!table->buckets[index])
        return 0;

    Identifier *node = table->buckets[index];
    int count = 0;
    while (node)
    {
        if (strings_equal(node->key, key))
            count++;
        node = node->next;
    }
    return count;
}

/**
 * Frees Identifier Table
 */
void free_IdentifierTable(IdentTable *table, bool free_rtobj)
{
    if (!table)
        return;

    for (int i = 0; i < table->bucket_count; i++)
    {
        Identifier *ptr = table->buckets[i];
        while (ptr)
        {
            Identifier *next = ptr->next;
            free_Identifier(ptr, free_rtobj);
            ptr = next;
        }
    }
    free(table->buckets);
    free(table);
}

/**
 * Below is the implementation of the stack machine used by the VM
 */

/**
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
    if (node->dispose && dispose)
        rtobj_free(node->obj, false);

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
 * Frees Stack Machine
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

/**
 * Below is the main logic loop for running a program
 */

/**
 * Important static declarations
 *
 */

/* This struct will store the runtime environment */
static RunTime *env = NULL;

/* Stores call stack */
static CallFrame *callStack[MAX_STACK_SIZE] = {NULL};

/* Flag for storing current status of runtime environment */
static bool Runtime_active = false;

StackMachine *getCurrentStkMachineInstance() { return env->stk_machine; }

#define disposable() env->stk_machine->head->dispose
#define StackMachine env->stk_machine

/* Returns Top Object on the stack */
#define TopStkMachineObject() env->stk_machine->head->obj
#define CurrentStackFrame() callStack[env->stack_ptr]

/**
 * Returns wether Runtime environment is currently running
 */
bool isRuntimeActive()
{
    return Runtime_active;
}

int getCallStackPointer()
{
    assert(env);
    return env->stack_ptr;
}

CallFrame **getCallStack()
{
    return callStack;
}

/**
 * Gets the Current StackFrame pointed to by the stack ptr
 */

CallFrame *getCurrentStackFrame() { return callStack[env->stack_ptr]; }

/**
 * DESCRIPTION:
 * Lookups variable in lookup table
 * 1- Checks if variable exists in current or higher scopes
 * 2- Checks if variable is a built in function
 *
 * PARAMS:
 * var: Variable name to lookup by
 *
 * NOTE:
 * If var is a builtin identifier, then a new RtObject is created.
 */
RtObject *lookup_variable(const char *var)
{
    RtObject *obj = IdentifierTable_get(callStack[getCallStackPointer()]->lookup, var);
    if (obj)
        return obj;

    RtObject *built_in = get_builtin_func(var);
    // add_to_GC_registry(built_in);

    return built_in;
}

/**
 * Mallocs Call Frame
 * function will be NULL if creating the global (top level) Call Frame
 */
CallFrame *init_CallFrame(ByteCodeList *program, RtFunction *function)
{
    CallFrame *cllframe = malloc(sizeof(CallFrame));
    // Maps strings to RtObjects
    cllframe->pg_counter = 0;
    cllframe->pg = program;
    cllframe->lookup = init_IdentifierTable();
    cllframe->function = function;
    return cllframe;
}

/**
 * Frees Call Frame
 * free_lookup_tb: wether lookup table should be freed
 */
void free_CallFrame(CallFrame *call, bool free_rtobj)
{
    free_IdentifierTable(call->lookup, free_rtobj);

    free(call);
}

/**
 * Sets up runtime
 */
RunTime *init_RunTime()
{
    RunTime *runtime = malloc(sizeof(RunTime));
    if (!runtime)
    {
        printf("Failed to allocate memory for Runtime Environment\n");
        return NULL;
    }
    runtime->stack_ptr = -1;
    Runtime_active = true;

    // creates stack machine
    runtime->stk_machine = init_StackMachine();
    if (!runtime->stk_machine)
    {
        free(runtime);
        printf("Failed to initialize Runtime Stack Machine\n");
        return NULL;
    }

    return runtime;
}

/* Frees Runtime */
void free_RunTime(RunTime *rt)
{
    // objects are gonna be freed by the GC
    free_StackMachine(rt->stk_machine, false);
    free(rt);
}

/**
 *  Pushes call frame to runtime call stack
 * */
void RunTime_push_callframe(CallFrame *frame)
{
    callStack[++env->stack_ptr] = frame;
}

/**
 * Pops Call frame from call stack
 */
CallFrame *RunTime_pop_callframe()
{
    assert(env);
    CallFrame *frame = callStack[env->stack_ptr];
    callStack[env->stack_ptr] = NULL;
    env->stack_ptr--;
    return frame;
}

/**
 * This Function setups runtime environment
 * It initializes the stack machine
 * Adds top level Call frame
 * The runtime struct
 * Std lib functions
 */
int prep_runtime_env(ByteCodeList *code)
{
    env = init_RunTime();
    RunTime_push_callframe(init_CallFrame(code, NULL));
    init_GarbageCollector();

    return env ? 1 : 0;
}

/**
 * DESCRIPTION:
 * This function is helper function for disposing objects,
 * if the object is disposable, then its freed, otherwise, if its NOT disposable
 * it must continue to live in the runtime, therefor its added to GC registry
 *
 * PARAMS:
 * obj: runtime object
 * disposable: wether object should be disposed or not
 */
static void dispose_disposable_obj(RtObject *obj, bool disposable)
{
    // frees Objects if they are disposable
    if (disposable)
    {
        remove_from_GC_registry(obj, true);
    }
    else
    {
        add_to_GC_registry(obj);
    }
}
/**
 * Helper function for performing primitive arithmetic operations (+, *, /, -, %, >>, <<)
 */
static void perform_binary_operation(
    RtObject *(*op_function)(RtObject *obj1, RtObject *obj2))
{
    assert(op_function);

    bool free_interm_1 = disposable();
    RtObject *intermediate1 = StackMachine_pop(StackMachine, false);
    bool free_interm_2 = disposable();
    RtObject *intermediate2 = StackMachine_pop(StackMachine, false);

    StackMachine_push(StackMachine, op_function(intermediate2, intermediate1), true);

    dispose_disposable_obj(intermediate1, free_interm_1);
    dispose_disposable_obj(intermediate2, free_interm_2);
}

/**
 * DESCRIPTION:
 * Helper function for variable mutations
 *
 *
 */
static void perform_var_mutation()
{

    bool new_val_disposable = disposable();
    RtObject *new_val = StackMachine_pop(env->stk_machine, false);
    bool mutable_disposable = disposable();
    RtObject *old_val = StackMachine_pop(env->stk_machine, false);

    // mutates data
    // if new_val is disposable, then a deep cpy is created, since new_val will be freed
    rtobj_mutate(old_val, new_val, new_val_disposable);

    // Makes sure new_val is either disposed or put into the GC registry
    // which it should already be if its not disposable
    dispose_disposable_obj(new_val, new_val_disposable);

    // makes sure old_val is put into the GC registry, (it should already be in it)
    add_to_GC_registry(old_val);
}

/**
 * DESCRIPTION:
 * Contains logic for loading a variable reference from a lookup table and pushing it onto the stack machine
 *
 * PARAMS:
 * table: lookup table
 * var: identifier to lookup
 */
static void perform_load_var(char *varname)
{
    assert(varname);
    RtObject *var = lookup_variable(varname);
    assert(var);
    // looked up variables are always

    // if its a built in function reference, then the object is disposable by default
    bool dispose = !IdentifierTable_get(callStack[getCallStackPointer()]->lookup, varname);
    StackMachine_push(StackMachine, var, dispose);
}

/**
 * DESCRIPTION:
 * Helper function for performing conditional jumps
 *
 * PARAMS:
 * offset: offset that will be jumped
 * condition: wether should eval to true or false for jump to occur
 * pop_stk: wether object should be popped from stack machine or not
 */
static void perform_conditional_jump(int offset, bool condition, bool pop_stk)
{
    CallFrame *frame = getCurrentStackFrame();
    bool dispose = disposable();
    RtObject *obj;
    if (pop_stk)
        obj = StackMachine_pop(env->stk_machine, false);
    else
        obj = TopStkMachineObject();

    // preforms jump if condition is met
    if (eval_obj(obj) == condition)
        frame->pg_counter += offset;
    else
        frame->pg_counter++;

    dispose_disposable_obj(obj, dispose);
}

/**
 * Handles Logic for CREATE_FUNCTION
 */
static void perform_create_function(RtObject *function)
{
    assert(function->type == FUNCTION_TYPE);
    RtObject *func = rtobj_shallow_cpy(function);

    // adds closure variable objects to function objects
    if (func->data.Func->func_data.user_func.closure_count > 0)
    {
        RtObject **closures = malloc(sizeof(RtObject *) * func->data.Func->func_data.user_func.closure_count);
        for (unsigned int i = 0; i < func->data.Func->func_data.user_func.closure_count; i++)
        {
            char *name = func->data.Func->func_data.user_func.closures[i];
            closures[i] = lookup_variable(name);
            assert(closures[i]);
        }
        func->data.Func->func_data.user_func.closure_obj = closures;
    }
    else
    {
        func->data.Func->func_data.user_func.closure_obj = NULL;
    }

    // Function Object can be disposable, since function data (stuff embedded within the bytecode)
    // is never freed
    StackMachine_push(StackMachine, func, true);
}

/**
 * Perform the logic for EXIT_PROGRAM
 * Returns the return code
 */

static int perform_exit()
{
    bool dispose = disposable();
    RtObject *obj = StackMachine_pop(env->stk_machine, false);
    if (obj->type != NUMBER_TYPE)
    {
        printf("Program cannot return %s\n", rtobj_type_toString(obj));
        return 1;
    }

    int return_code = (int)obj->data.Number.number;
    dispose_disposable_obj(obj, dispose);
    free_CallFrame(RunTime_pop_callframe(), false);

    return return_code;
}

/**
 * Logic performing function calls
 * This function will return null if function call was built in
 */

static void perform_builtin_call(Builtin *builtin, RtObject **args, int arg_count);
static CallFrame *perform_regular_func_call(RtFunction *function, RtObject **arguments, unsigned int arg_count);

CallFrame *perform_function_call(unsigned int arg_count)
{

    // gets the arguments
    RtObject *arguments[arg_count];

    bool disposable[arg_count];
    // adds arguments to registry
    for (int i = arg_count - 1; i >= 0; i--)
    {
        disposable[i] = disposable();
        arguments[i] = StackMachine_pop(env->stk_machine, false);
        if (!disposable[i])
            add_to_GC_registry(arguments[i]);
    }

    bool func_disposable = disposable();
    RtObject *func = StackMachine_pop(env->stk_machine, false);

    // called object is not a function
    if (func->type != FUNCTION_TYPE)
    {
        printf("Object is not callable\n");
        return NULL;
        // argument count does not match
    }

    // temporary for now
    if (env->stack_ptr >= MAX_STACK_SIZE - 1 && !func->data.Func->is_builtin)
    {

        printf("Stack Overflow Error when calling function '%s' \n",
               func->data.Func->func_data.user_func.func_name);
        exit(2);
        return NULL;
    }

    CallFrame *new_frame = NULL;

    // built in function
    if (func->data.Func->is_builtin)
    {
        perform_builtin_call(func->data.Func->func_data.built_in.func, arguments, arg_count);
        for (unsigned int i = 0; i < arg_count; i++)
            dispose_disposable_obj(arguments[i], disposable[i]);
    }
    else
    {
        new_frame = perform_regular_func_call(func->data.Func, arguments, arg_count);
    }

    dispose_disposable_obj(func, func_disposable);

    return new_frame;
}

/**
 * Helper for performing built in function call
 * Adds new Callframe to call stack
 */
static void perform_builtin_call(Builtin *builtin, RtObject **args, int arg_count)
{
    assert(builtin);

    // checks argument counts match
    if (
        builtin->arg_count != arg_count &&
        // if func takes a finite number of args, if its set to -1, then it could take any number of args
        builtin->arg_count >= 0)
    {

        printf("Built in function '%s' expects %d arguments, but got %d\n",
               builtin->builtin_name,
               builtin->arg_count,
               arg_count);
        return;
    }

    // pushes result of function onto stack machine
    StackMachine_push(StackMachine, builtin->builtin_func((const RtObject **)args, arg_count), true);
}

/**
 * Helper for performing logic for handling function calls
 */
static CallFrame *perform_regular_func_call(RtFunction *Function, RtObject **arguments, unsigned int arg_count)
{

    // checks argument counts match
    if (!Function->is_builtin &&
        Function->func_data.user_func.arg_count != arg_count)
    {
        printf("Function '%s' expected %d arguments, but got %d\n",
               Function->func_data.user_func.func_name,
               Function->func_data.user_func.arg_count,
               arg_count);
        return NULL;
    }

    CallFrame *new_frame = init_CallFrame(Function->func_data.user_func.body, Function);

    // adds function arguments to lookup table and ref list
    for (unsigned int i = 0; i < arg_count; i++)
    {
        Identifier_Table_add_var(
            new_frame->lookup,
            Function->func_data.user_func.args[i],
            arguments[i],
            DOES_NOT_APPLY);
    }

    // adds closure variables
    for (unsigned int i = 0; i < Function->func_data.user_func.closure_count; i++)
    {
        Identifier_Table_add_var(
            new_frame->lookup,
            Function->func_data.user_func.closures[i],
            // lookup_variable(Function->func_data.user_func.func_name)
            Function->func_data.user_func.closure_obj[i],
            DOES_NOT_APPLY);
    }

    // adds function definition to lookup (to allow recursion)
    // thats why we perform a shallow copy
    if (Function->func_data.user_func.func_name)
    {
        RtObject *cpy_func = init_RtObject(FUNCTION_TYPE);
        cpy_func->data.Func = rtfunc_cpy(Function, true);

        Identifier_Table_add_var(
            new_frame->lookup,
            Function->func_data.user_func.func_name,
            cpy_func,
            DOES_NOT_APPLY);

        // call frame is pushed before adding func to GC registry
        RunTime_push_callframe(new_frame);
        add_to_GC_registry(cpy_func);
    }
    else
    {
        // add new call frame to call stack
        RunTime_push_callframe(new_frame);
    }

    return new_frame;
}

/**
 * DESCRIPTION:
 * Creates list of given length by popping elements from the stack machine, new list object is pushed on to the stack machine
 *
 * PARAMS:
 * length: length of the list
 */
static void perform_create_list(unsigned long length)
{
    RtObject *listobj = init_RtObject(LIST_TYPE);
    RtObject *tmp[length];

    for (unsigned long i = 0; i < length; i++)
    {
        bool disposable = disposable();
        RtObject *obj = StackMachine_pop(StackMachine, false);
        assert(obj);

        // if object is not disposable, then a shallow copy is created
        // since its not disposable, it means its already in the GC registry, so it can be discarded
        tmp[i] = disposable ? obj : rtobj_shallow_cpy(obj);
    }

    RtList *list = init_RtList(length >= DEFAULT_RTLIST_LEN ? length * 2 : DEFAULT_RTLIST_LEN);
    for (int i = (int)length - 1; i >= 0; i--)
    {
        rtlist_append(list, tmp[i]);
    }

    listobj->data.List = list;
    StackMachine_push(StackMachine, listobj, true);
    add_to_GC_registry(listobj);

    for (unsigned long i = 0; i < list->length; i++)
    {
        add_to_GC_registry(list->objs[i]);
    }
}

/**
 * DESCRIPTION:
 * Creates a map and pushes it to the stack machine
 *
 * PARAMS:
 * size: refers to the number of key value pairs
 */
static void perform_create_map(unsigned long size)
{

    RtObject *keys[size / 2];
    RtObject *values[size / 2];

    RtMap *map = init_RtMap(size / 2);

    // fetches all maps and keys
    for (unsigned long i = 0; i < size; i++)
    {
        bool valdispose = disposable();
        RtObject *val = StackMachine_pop(StackMachine, false);
        bool keydispose = disposable();
        RtObject *key = StackMachine_pop(StackMachine, false);

        rtmap_insert(
            map,
            keydispose ? key : rtobj_shallow_cpy(key),
            valdispose ? val : rtobj_shallow_cpy(val));

        add_to_GC_registry(val);
        add_to_GC_registry(key);
    }
    RtObject *mapobj = init_RtObject(HASHMAP_TYPE);
    mapobj->data.Map = map;

    add_to_GC_registry(mapobj);

    StackMachine_push(StackMachine, mapobj, true);
}

/**
 * DESCRIPTION:
 * Fetches object from somce other object, using an index object
 */
static void perform_get_index()
{
    bool index_disposable = disposable();
    RtObject *index_ = StackMachine_pop(StackMachine, false);
    bool obj_disposable = disposable();
    RtObject *obj = StackMachine_pop(StackMachine, false);

    RtObject *indexed_obj = rtobj_getindex(obj, index_);

    dispose_disposable_obj(index_, index_disposable);
    dispose_disposable_obj(obj, obj_disposable);

    // object was fetched from an other object, therefor it should be disposed, since the latter has ref to it
    StackMachine_push(StackMachine, indexed_obj, false);
}

/**
 * DESCRIPTION:
 * Creates an class object and pushes on the stack
 */
static void perform_return_class()
{
    Identifier **fields = IdentifierTable_to_IdentList(CurrentStackFrame()->lookup);

    RtClass *cl = init_RtClass(
        getCurrentStackFrame()->function,
        getCurrentStackFrame()->function->func_data.user_func.func_name);

    if(!cl) MallocError();

    for (unsigned int i = 0; fields[i] != NULL; i++)
    {
        if(fields[i]->access != PUBLIC_ACCESS) continue;
        RtObject *attrname = init_RtObject(STRING_TYPE);
        attrname->data.String.string = cpy_string(fields[i]->key);
        rtmap_insert(cl->attrs_table, attrname, fields[i]->obj);
    }

    RtObject *class = init_RtObject(CLASS_TYPE);
    class->data.Class = cl;
    
    StackMachine_push(StackMachine, class, true);
}
/******************************************************/

/**
 * Performs a logic for creating variables
 */
void perform_create_var(char *varname, AccessModifier access)
{
    bool disposable = disposable();
    RtObject *new_val = StackMachine_pop(StackMachine, false);
    CallFrame *frame = getCurrentStackFrame();

    RtObject *cpy;
    if (disposable)
        cpy = new_val;
    else
        cpy = rtobj_shallow_cpy(new_val);

    Identifier_Table_add_var(frame->lookup, varname, cpy, access);

    // if(!disposable)
    add_to_GC_registry(cpy);
}

int run_program()
{
    assert(env);
    while (1)
    {
        CallFrame *frame = callStack[env->stack_ptr];
        ByteCodeList *bytecode = frame->pg;
        while (1)
        {
            ByteCode *code = bytecode->code[frame->pg_counter];
            bool loop = true;

            switch (code->op_code)
            {
            case LOAD_CONST:
            {
                // contants are imbedded within the bytecode
                // therefor a deep copy is created
                StackMachine_push(StackMachine, rtobj_deep_cpy(code->data.LOAD_CONST.constant), true);
                break;
            }

            case ADD_VARS_OP:
            {
                perform_binary_operation(add_objs);
                break;
            }

            case SUB_VARS_OP:
            {
                perform_binary_operation(substract_objs);
                break;
            }

            case MULT_VARS_OP:
            {
                perform_binary_operation(multiply_objs);
                break;
            }

            case DIV_VARS_OP:
            {
                perform_binary_operation(divide_objs);
                break;
            }

            case MOD_VARS_OP:
            {
                perform_binary_operation(modulus_objs);
                break;
            }

            case EXP_VARS_OP:
            {
                perform_binary_operation(exponentiate_obj);
                break;
            }

            case BITWISE_VARS_AND_OP:
            {
                perform_binary_operation(bitwise_and_objs);
                break;
            }

            case BITWISE_VARS_OR_OP:
            {
                perform_binary_operation(bitwise_or_objs);
                break;
            }

            case BITWISE_XOR_VARS_OP:
            {
                perform_binary_operation(bitwise_xor_objs);
                break;
            }

            case SHIFT_LEFT_VARS_OP:
            {
                perform_binary_operation(shift_left_objs);
                break;
            }

            case SHIFT_RIGHT_VARS_OP:
            {
                perform_binary_operation(shift_right_objs);
                break;
            }

            case GREATER_THAN_VARS_OP:
            {
                perform_binary_operation(greater_than_op);
                break;
            }

            case GREATER_EQUAL_VARS_OP:
            {
                perform_binary_operation(greater_equal_op);
                break;
            }

            case LESSER_THAN_VARS_OP:
            {
                perform_binary_operation(lesser_than_op);
                break;
            }

            case LESSER_EQUAL_VARS_OP:
            {
                perform_binary_operation(lesser_equal_op);
                break;
            }

            case EQUAL_TO_VARS_OP:
            {
                perform_binary_operation(equal_op);
                break;
            }

            case LOGICAL_AND_VARS_OP:
            {
                perform_binary_operation(logical_and_op);
                break;
            }

            case LOGICAL_OR_VARS_OP:
            {
                perform_binary_operation(logical_or_op);
                break;
            }

            case LOGICAL_NOT_VARS_OP:
            {
                logical_not_op(StackMachine->head->obj);
                break;
            }

            case CREATE_VAR:
            {
                perform_create_var(code->data.CREATE_VAR.new_var_name, code->data.CREATE_VAR.access);
                break;
            }

            case LOAD_VAR:
            {
                perform_load_var(code->data.LOAD_VAR.variable);
                break;
            }

            // dereferences variable
            case DEREF_VAR:
            {
                Identifier_Table_remove_var(frame->lookup, code->data.DEREF_VAR.var);
                break;
            }

            case MUTATE_VAR:
            {
                perform_var_mutation();
                break;
            }

            case FUNCTION_CALL:
            {
                // checks wether a new call frame was created
                // if it was, then it ends the inner loop
                // otherwise, it was a built in function, in which case we continue the inner loop as usual
                if (perform_function_call(code->data.FUNCTION_CALL.arg_count))
                {
                    loop = false; // ends loop
                }

                break;
            }

            case OFFSET_JUMP_IF_FALSE_POP:
            {
                perform_conditional_jump(code->data.OFFSET_JUMP_IF_FALSE_POP.offset, false, true);
                continue;
            }

            case OFFSET_JUMP_IF_TRUE_POP:
            {
                perform_conditional_jump(code->data.OFFSET_JUMP_IF_TRUE_POP.offset, true, true);
                continue;
            }

            case OFFSET_JUMP_IF_FALSE_NOPOP:
            {
                perform_conditional_jump(code->data.OFFSET_JUMP_IF_FALSE_NOPOP.offset, false, false);
                continue;
            }

            case OFFSET_JUMP_IF_TRUE_NOPOP:
            {
                perform_conditional_jump(code->data.OFFSET_JUMP_IF_TRUE_NOPOP.offset, true, false);
                continue;
            }

            case OFFSET_JUMP:
            {
                CurrentStackFrame()->pg_counter += code->data.OFFSET_JUMP.offset;
                continue;
            }

            case CREATE_FUNCTION:
            {
                perform_create_function(code->data.CREATE_FUNCTION.function);
                break;
            }

            case FUNCTION_RETURN_UNDEFINED:
            {
                StackMachine_push(StackMachine, init_RtObject(UNDEFINED_TYPE), true);
                CallFrame *poppedFrame = RunTime_pop_callframe();
                free_CallFrame(poppedFrame, false);
                loop = false;
                getCurrentStackFrame()->pg_counter++;
                break;
            }

            // any return value, if present, will be on the stack already
            case FUNCTION_RETURN:
            {
                assert(TopStkMachineObject());
                free_CallFrame(RunTime_pop_callframe(), false);
                loop = false;
                getCurrentStackFrame()->pg_counter++;
                break;
            }

            case CREATE_OBJECT_RETURN:
            {
                perform_return_class();
                free_CallFrame(RunTime_pop_callframe(), false);
                loop = false;
                getCurrentStackFrame()->pg_counter++;
                break;
            }

            case LOAD_ATTRIBUTE: {
                // VERY TEMPORARY CODE
                RtObject *tmp = StackMachine_pop(StackMachine, false);

                RtObject *attrs = init_RtObject(STRING_TYPE);
                attrs->data.String.string = cpy_string(code->data.LOAD_ATTR.attribute_name);
                attrs->data.String.string_length = code->data.LOAD_ATTR.str_length;

                StackMachine_push(StackMachine, rtmap_get(tmp->data.Class->attrs_table, attrs), false);

                rtobj_free(attrs, false);
                break;
            }

            case POP_STACK:
            {
                StackMachine_pop(StackMachine, true);
                break;
            }

            case CREATE_LIST:
            {
                perform_create_list(code->data.CREATE_LIST.list_length);
                break;
            }

            case CREATE_MAP:
            {
                perform_create_map(code->data.CREATE_MAP.map_size);
                break;
            }

            case LOAD_INDEX:
            {
                perform_get_index();
                break;
            }

            case EXIT_PROGRAM:
            {
                return perform_exit();
            }

                // TODO
                // .
                // .
                // .
                //
                //
            }

            if (!loop)
                break;

            frame->pg_counter++;
            trigger_GC();
        }
    }
    return 0;
}

/**
 * This function performs cleanup
 *
 * */
void perform_cleanup()
{
    if (env)
    {
        free_RunTime(env);
        env = NULL;
    }

    cleanup_builtin();
    cleanup_GarbageCollector();
    Runtime_active = false;
}