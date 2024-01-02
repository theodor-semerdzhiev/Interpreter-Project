#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../generics/utilities.h"
#include "../compiler/compiler.h"
#include "builtins.h"
#include "runtime.h"


/**
 * DESCRIPTION:
 * This File contains the implementation of the runtime environment.
 * Includes:
 * - Main code loop
 * - Computation Stack
 *
 */

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
static Identifier *init_Identifier(const char *varname, RtObject *obj)
{
    Identifier *node = malloc(sizeof(Identifier));
    if (!node)
        return NULL;
    node->obj = obj;
    node->key = malloc_string_cpy(NULL, varname);
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
        free_RtObject(node->obj, false);

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
void Identifier_Table_add_var(IdentTable *table, const char *key, RtObject *obj)
{
    assert(table);
    unsigned int index = djb2_string_hash(key) % table->bucket_count;
    if (!table->buckets[index])
    {
        table->buckets[index] = init_Identifier(key, obj);
    }
    else
    {
        Identifier *node = init_Identifier(key, obj);
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

typedef struct StkMachineNode StkMachineNode;

typedef struct StkMachineNode
{
    RtObject *obj;
    StkMachineNode *next;
    int dispose; // wether object should be freed when popped
} StkMachineNode;

typedef struct StackMachine
{
    StkMachineNode *head;
} StackMachine;

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
        free_RtObject(node->obj, false);

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
    stk_machine->head = popped->next;
    if (dispose)
    {
        free_StackMachineNode(popped, true);
        return NULL;
    }
    else
    {
        RtObject *obj = popped->obj;
        free_StackMachineNode(popped, false);
        return obj;
    }
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
    return obj;
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
            free_RtObject(tmp->obj, false);
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
CallFrame *callStack[MAX_STACK_SIZE] = {NULL};

/* Flag for storing current status of runtime environment */
static bool Runtime_active = false;

#define disposable() env->stk_machine->head->dispose
#define StackMachine env->stk_machine
#define CurrentStackFrame callStack[env->stack_ptr];

/**
 * Returns wether Runtime environment is currently running
 */
bool isRuntimeActive()
{
    return Runtime_active;
}

/**
 * Gets the Current StackFrame pointed to by the stack ptr
 */

CallFrame *getCurrentStackFrame()
{
    return callStack[env->stack_ptr];
}

/**
 * Lookups variable in lookup table
 * 1- Checks if variable exists in current or higher scopes
 * 2- Checks if variable is a built in function
 */
RtObject *lookup_variable(const char *var)
{
    for (int i = env->stack_ptr; i >= 0; i--)
    {
        RtObject *obj = IdentifierTable_get(callStack[i]->lookup, var);
        if (obj)
            return obj;
    }

    RtObject *built_in = get_builtin_func(var);

    return built_in;
}

/**
 * Mallocs Call Frame
 * function will be NULL if creating the global (top level) Call Frame
 */
CallFrame *init_CallFrame(ByteCodeList *program, RtObject *function)
{
    assert(!function || function->type == FUNCTION_TYPE);

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
    Runtime_active=true;

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

    return env ? 1 : 0;
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

    // frees Objects if they are disposable
    if (free_interm_1)
        free_RtObject(intermediate1, false);
    if (free_interm_2)
        free_RtObject(intermediate2, false);
}

/**
 * Helper function for performing logic for mutating variables
 */
static void perform_var_mutation()
{

    bool new_val_disposable = disposable();
    RtObject *new_val = StackMachine_pop(env->stk_machine, false);
    bool mutable_disposable = disposable();
    RtObject *mutable = StackMachine_pop(env->stk_machine, false);

    // if the new_val is disposable, then it will be freed
    // Therefor, a deep cpy mutation must performed
    mutate_obj(mutable, new_val, new_val_disposable);

    if (new_val_disposable)
        free_RtObject(new_val, false);
    if (mutable_disposable)
        free_RtObject(mutable, false);
}

/**
 * Helper function for performing conditional jumps
 * offset: offset that will be jumped
 * condition: wether should eval to true or false for jump to occure
 */
static void perform_conditional_jump(int offset, bool condition)
{
    CallFrame *frame = getCurrentStackFrame();
    bool dispose = disposable();
    RtObject *obj = StackMachine_pop(env->stk_machine, false);

    // preforms jump if condition is met
    if (eval_obj(obj) == condition)
        frame->pg_counter += offset;
    else
        frame->pg_counter++;

    // frees object if its disposable
    if (dispose)
        free_RtObject(obj, false);
}

/**
 * Handles Logic for CREATE_FUNCTION
 */
static void perform_create_function(RtObject *function)
{
    assert(function->type == FUNCTION_TYPE);
    RtObject *func = shallow_cpy_rtobject(function);

    // adds closure variable objects to function objects
    if (func->data.Function.func_data.user_func.closure_count > 0)
    {
        RtObject **closures = malloc(sizeof(RtObject *) * func->data.Function.func_data.user_func.closure_count);
        for (int i = 0; i < func->data.Function.func_data.user_func.closure_count; i++)
        {
            char *name = func->data.Function.func_data.user_func.closures[i];
            closures[i] = lookup_variable(name);
            assert(closures[i]);
            add_ref(func, closures[i]);
        }
        func->data.Function.func_data.user_func.closure_obj = closures;
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
        printf("Program cannot return %s\n", obj_type_toString(obj));
        return 1;
    }

    int return_code = (int)obj->data.Number.number;
    if (dispose)
    {
        free_RtObject(obj, false);
    }

    return return_code;
}

/**
 * Logic performing function calls
 * This function will return null if function call was built in
 */

static void perform_builtin_call(RtObject *builtin, RtObject **args, int arg_count);
static CallFrame *perform_regular_func_call(RtObject *function, RtObject **arguments, int arg_count);

CallFrame *perform_function_call(int arg_count)
{

    // gets the arguments
    RtObject *arguments[arg_count];
    // bool dispose_arg[arg_count];
    for (int i = arg_count - 1; i >= 0; i--) {
        // dispose_arg[i] = disposable();
        arguments[i] = StackMachine_pop(env->stk_machine, false);
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
    if(env->stack_ptr >= MAX_STACK_SIZE && !func->data.Function.is_builtin) {

        printf("Stack Overflow Error when calling function '%s' \n", 
        func->data.Function.func_data.user_func.func_name);
        exit(2);
        return NULL;
    }

    CallFrame *new_frame = NULL;
    // built in function
    if (func->data.Function.is_builtin)
    {
        perform_builtin_call(func, arguments, arg_count);

        // regular function
    }
    else
    {
        new_frame = perform_regular_func_call(func, arguments, arg_count);
    }

    // frees function if its disposable
    if (func_disposable)
        free_RtObject(func, false);
    
    // frees function objects if disposable
    // for(int i=0 ; i <arg_count; i++) {
    //     if(dispose_arg[i]) free_RtObject(arguments[i], false);
    // }

    
    return new_frame;
}

/**
 * Helper for performing built in function call
 * Adds new Callframe to call stack
 */
static void perform_builtin_call(RtObject *builtin, RtObject **args, int arg_count)
{
    assert(builtin->data.Function.is_builtin);

    // checks argument counts match
    if (
        (!builtin->data.Function.is_builtin &&
         builtin->data.Function.func_data.built_in.func->arg_count != arg_count) &&
        // wether func takes a finite number of args
        builtin->data.Function.func_data.built_in.func->arg_count >= 0)
    {

        printf("Built in function '%s' expects %d arguments, but got %d\n",
               builtin->data.Function.func_data.built_in.func->builtin_name,
               builtin->data.Function.func_data.built_in.func->arg_count,
               arg_count);
        return;
    }

    RtObject *(*func)(const RtObject **, int) = builtin->data.Function.func_data.built_in.func->builtin_func;
    // pushes result of function onto stack machine
    StackMachine_push(StackMachine, func((const RtObject **)args, arg_count), true);
}

/**
 * Helper for performing logic for handling function calls
 */
static CallFrame *perform_regular_func_call(RtObject *function, RtObject **arguments, int arg_count)
{
    assert(!function->data.Function.is_builtin);
    // checks argument counts match
    if (!function->data.Function.is_builtin &&
        function->data.Function.func_data.user_func.arg_count != arg_count)
    {
        printf("Function expected %d arguments, but got %d\n", function->data.Function.func_data.user_func.arg_count, arg_count);
        return NULL;
    }

    CallFrame *new_frame = init_CallFrame(function->data.Function.func_data.user_func.body, function);
    // adds function arguments to lookup table and ref list
    for (int i = 0; i < arg_count; i++)
    {
        Identifier_Table_add_var(
            new_frame->lookup,
            function->data.Function.func_data.user_func.args[i],
            arguments[i]);

        // add_ref(func, arguments[i]);
    }

    // adds closure variables
    for (int i = 0; i < function->data.Function.func_data.user_func.closure_count; i++)
    {
        Identifier_Table_add_var(
            new_frame->lookup,
            function->data.Function.func_data.user_func.closures[i],
            function->data.Function.func_data.user_func.closure_obj[i]);
    }

    // adds function definition to lookup (to allow recursion), if applicable, 
    // thats why we perform a shallow copy
    if (function->data.Function.func_data.user_func.func_name)
        Identifier_Table_add_var(
            new_frame->lookup,
            function->data.Function.func_data.user_func.func_name,
            shallow_cpy_rtobject(function));

    // add new call frame to call stack
    RunTime_push_callframe(new_frame);

    return new_frame;
}

/******************************************************/

/**
 * Performs a logic for creating variables
*/
void perform_create_var(char *varname) {
    bool disposable = disposable();
    RtObject *new_val = StackMachine_pop(StackMachine, false);
    CallFrame *frame = getCurrentStackFrame();
    Identifier_Table_add_var(
        frame->lookup,
        varname,
        // If its disposable, then object associated data will be freed
        // leading to double free error, therefor we create deep cope
        // otherwise its not problem
        disposable ? deep_cpy_rtobject(new_val) : shallow_cpy_rtobject(new_val));

    if (disposable)
        free_RtObject(new_val, false);
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
                StackMachine_push(StackMachine, deep_cpy_rtobject(code->data.LOAD_CONST.constant), true);
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
                perform_create_var(code->data.CREATE_VAR.new_var_name);
                break;
            }

            case LOAD_VAR:
            {
                // looked up variables are always
                StackMachine_push(StackMachine, lookup_variable(code->data.LOAD_VAR.variable), false);
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

            case OFFSET_JUMP_IF_FALSE:
            {
                perform_conditional_jump(code->data.OFFSET_JUMP_IF_FALSE.offset, false);
                continue;
            }

            case OFFSET_JUMP_IF_TRUE:
            {
                perform_conditional_jump(code->data.OFFSET_JUMP_IF_FALSE.offset, true);
                continue;
            }

            case OFFSET_JUMP:
            {
                getCurrentStackFrame()->pg_counter += code->data.OFFSET_JUMP.offset;
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
                free_CallFrame(RunTime_pop_callframe(), false);
                loop = false;
                getCurrentStackFrame()->pg_counter++;
                break;
            }

            case POP_STACK:
            {
                StackMachine_pop(StackMachine, true);
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

            default:
                break;
            }

            if (!loop)
                break;

            frame->pg_counter++;
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
    Runtime_active = false;
}