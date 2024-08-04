#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../generics/utilities.h"
#include "../compiler/compiler.h"
#include "../rtlib/builtinfuncs.h"
#include "runtime.h"
#include "identtable.h"
#include "rtlists.h"
#include "rttype.h"
#include "filetable.h"
#include "gc.h"
#include "rtexchandler.h"

/**
 * DESCRIPTION:
 * This File contains the implementation of the runtime environment.
 */

/**
 * Important static declarations
 *
 */

/* Stores call stack */
static CallFrame *callStack[MAX_STACK_SIZE] = {NULL};

size_t MAX_CALLSTACK_SIZE = MAX_STACK_SIZE;

/* Flag for storing current status of runtime environment */
static bool Runtime_active = false;

/* Stack machine */
static StackMachine *stk_machine;

/* Stack Call Frame pointer */
static long stack_ptr = -1;

StackMachine *getCurrentStkMachineInstance() { return stk_machine; }

#define disposable() stk_machine->head->dispose
#define StackMachine stk_machine

/* Returns Top Object on the stack */
#define TopStkMachineObject() stk_machine->head->obj
#define CurrentStackFrame() callStack[stack_ptr]

/**
 * Returns wether Runtime environment is currently running
 */
bool isRuntimeActive()
{
    return Runtime_active;
}

long getCallStackPointer()
{
    return stack_ptr;
}

CallFrame **getCallStack()
{
    return callStack;
}

/**
 * Gets the Current StackFrame pointed to by the stack ptr
 */

CallFrame *getCurrentStackFrame()
{
    return callStack[stack_ptr];
}

/**
 * DESCRITPION:
 * Sets up runtime, by intializing static closures
 */
int init_RunTime()
{
    stack_ptr = -1;
    Runtime_active = true;

    // creates stack machine
    stk_machine = init_StackMachine();
    if (!stk_machine)
    {
        printf("Failed to initialize Runtime Stack Machine\n");
        return 0;
    }

    return 1;
}

/**
 * 
 * DESCRIPTION:
 * Initializes script argument array and adds a variable called _args
*/
void init_ScriptArgs(int argc, char **argv) {
    CallFrame *frame = getCurrentStackFrame();
    assert(frame);

    RtObject *arglist = init_RtObject(LIST_TYPE);
    arglist->data.List = init_RtList(argc);

    for(int i=0; i < argc; i++) {
        RtObject *arg = init_RtObject(STRING_TYPE);
        arg->data.String = init_RtString(argv[i]);
        add_to_GC_registry(arg);
        
        rtlist_append(arglist->data.List, arg);
    }

    Identifier_Table_add_var(frame->lookup, BUILT_IN_SCRIPT_ARGS_VAR, arglist, DOES_NOT_APPLY);
    add_to_GC_registry(arglist);
}

/**
 * DESCRIPTION:
 * This Function setups runtime environment
 * It initializes the stack machine
 * Adds top level Call frame
 * The runtime struct
 * Std lib functions
 */
int prep_runtime_env(ByteCodeList *code, const char *mainfile, int argc, char **argv)
{
    int returncode = init_RunTime();
    RunTime_push_callframe(init_CallFrame(code, NULL, mainfile));
    init_GarbageCollector();
    init_AttrRegistry();
    init_FileTable();
    rtobj_init_cmp_tbl();
    init_ScriptArgs(argc, argv);
    return returncode ? 1 : 0;
}

/**
 * DESCRIPTION:
 * This function performs runtime cleanup
 * After Program execution
 * */
void perform_runtime_cleanup()
{
    stack_ptr = -1;

    free_StackMachine(stk_machine, true, false);
    stk_machine = NULL;

    cleanup_builtin();
    cleanup_GarbageCollector();
    cleanup_AttrsRegistry();
    cleanup_FileTable();
    
    rtexception_free(raisedException);
    raisedException = NULL;

    Runtime_active = false;
}

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

    RtObject *built_in = get_builtinfunc(var);

    return built_in;
}

/**
 * Mallocs Call Frame
 * function will be NULL if creating the global (top level) Call Frame
 */
CallFrame *init_CallFrame(
    ByteCodeList *program,
    RtFunction *function,
    const char *filename)
{
    assert(program);

    CallFrame *cllframe = malloc(sizeof(CallFrame));
    // Maps strings to RtObjects
    cllframe->pg_counter = 0;
    cllframe->pg = program;
    cllframe->lookup = init_IdentifierTable();
    cllframe->function = function;
    cllframe->exception_jump = malloc(sizeof(jmp_buf));
    cllframe->code_file_location = cpy_string(filename);
    return cllframe;
}

/**
 * DESCRIPTION:
 * Frees Call Frame
 *
 * PARAMS:
 * free_rtobj: wether wether rtobj data in lookup table should be freed
 *
 * NOTE:
 * Rtobjects themselves will always be freed
 */
void free_CallFrame(CallFrame *call, bool free_rtobj_data)
{
    free_IdentifierTable(call->lookup, free_rtobj_data);
    free(call->code_file_location);
    free(call->exception_jump);
    free(call);
}

/**
 *  Pushes call frame to runtime call stack
 * */
void RunTime_push_callframe(CallFrame *frame)
{
    callStack[++stack_ptr] = frame;
}

/**
 * Pops Call frame from call stack
 */
CallFrame *RunTime_pop_callframe()
{
    CallFrame *frame = callStack[stack_ptr];
    callStack[stack_ptr] = NULL;
    stack_ptr--;
    return frame;
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
void dispose_disposable_obj(RtObject *obj, bool disposable)
{
    // frees Objects if they are disposable
    if (disposable)
    {
        assert(rtobj_refcount(obj) == 0);
        rtobj_free(obj, false, true);
    }
    else
    {
        // add_to_GC_registry(obj);
        assert(GC_Registry_has(obj));
    }
}

/**
 * Helper function for performing primitive arithmetic operations (+, *, /, -, %, >>, <<)
 */
static void perform_binary_operation(
    RtObject *(*op_function)(RtObject *obj1, RtObject *obj2))
{
    assert(op_function);
    assert(!Intermediate_raisedException);

    bool free_interm_1 = disposable();
    RtObject *intermediate1 = StackMachine_pop(StackMachine, false);
    bool free_interm_2 = disposable();
    RtObject *intermediate2 = StackMachine_pop(StackMachine, false);

    RtObject *result = op_function(intermediate2, intermediate1);

    dispose_disposable_obj(intermediate1, free_interm_1);
    dispose_disposable_obj(intermediate2, free_interm_2);

    if (Intermediate_raisedException)
    {
        assert(!result);
        raiseException(Intermediate_raisedException);
        return;
    }
    StackMachine_push(StackMachine, result, true);
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
    RtObject *new_val = StackMachine_pop(stk_machine, false);
    bool old_val_disposable = disposable();
    RtObject *old_val = StackMachine_pop(stk_machine, false);

    assert(new_val && old_val);

    // if mutatble value is disposable, then mutation is redundant
    // since mutable value will get freed
    if (old_val_disposable)
    {
        dispose_disposable_obj(new_val, new_val_disposable);
        dispose_disposable_obj(old_val, old_val_disposable);
        return;
    }

    // Mutation only occurs data pointer are different
    // i.e If a change actually happens
    if (rtobj_getdata(old_val) != rtobj_getdata(new_val))
    {
        size_t refcount = rtobj_refcount(old_val);
        rtobj_refcount_decrement1(old_val);
        
        // variable mutation might lead to pointer loss
        // therefore data1 is put into the GC to maintain the ptr ref
        add_to_GC_registry(rtobj_shallow_cpy(old_val));
        
        rtobj_mutate(old_val, new_val, new_val_disposable);

        // old_val needs to have its reference count updated
        // to ensure correctness
        rtobj_increment_refcount(old_val, refcount);
    }

    if (new_val_disposable)
    {
        assert(!GC_Registry_has(new_val));
        rtobj_shallow_free(new_val);
    }

    dispose_disposable_obj(old_val, old_val_disposable);
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
    // if(!dispose)
    //     assert(GC_Registry_has(var));

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
        obj = StackMachine_pop(stk_machine, false);
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
static void perform_create_function(const RtObject *function)
{
    assert(function->type == FUNCTION_TYPE);
    assert(!function->data.Func->func_data.user_func.closure_obj);

    RtObject *func = rtobj_deep_cpy(function, false);
    assert(!func->data.Func->func_data.user_func.closure_obj);

    unsigned int closure_count = func->data.Func->func_data.user_func.closure_count;

    // adds closure variable objects to function objects
    if (closure_count > 0)
    {
        RtObject **closures = malloc(sizeof(RtObject *) * closure_count);
        for (unsigned int i = 0; i < closure_count; i++)
        {
            char *name = func->data.Func->func_data.user_func.closures[i];
            closures[i] = lookup_variable(name);

            // updates ref count
            rtobj_refcount_increment1(closures[i]);

            assert(GC_Registry_has(closures[i]));
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
    RtObject *obj = StackMachine_pop(stk_machine, false);
    if (obj->type != NUMBER_TYPE)
    {
        printf("Program cannot return %s\n", rtobj_type_toString(obj->type));
        return 1;
    }

    int return_code = (int)obj->data.Number->number;
    dispose_disposable_obj(obj, dispose);
    free_CallFrame(RunTime_pop_callframe(), false);
    return return_code;
}

/**
 * Logic performing function calls
 * This function will return null if function call was built in
 */

static CallFrame *perform_regular_func_call(RtObject *funcobj, bool funcobj_disposable, RtObject **arguments, bool disposable[], size_t arg_count);

CallFrame *perform_function_call(size_t arg_count)
{
    // gets the arguments
    RtObject *arguments[arg_count];

    bool arg_disposable[arg_count];

    // adds arguments to registry
    for (int i = arg_count - 1; i >= 0; i--)
    {
        arg_disposable[i] = disposable();
        RtObject *arg = StackMachine_pop(stk_machine, false);

        arguments[i] = rtobj_rt_preprocess(arg, arg_disposable[i], true);

        addDisposablePrimitiveToGC(arg_disposable[i], arguments[i]);

        if (!DisposableOrPrimitive(arg_disposable[i], arg))
        {
            assert(GC_Registry_has(arguments[i]));
        }
    }

    bool func_disposable = disposable();
    RtObject *func = StackMachine_pop(stk_machine, false);

    if (func_disposable)
        assert(!GC_Registry_has(func));
    else
        assert(GC_Registry_has(func));

    // called object is not a function exception
    if (func->type != FUNCTION_TYPE)
    {
        const char *type = rtobj_type_toString(func->type);
        char buffer[strlen(type) + 50];
        snprintf(buffer, sizeof(buffer), "Object of type %s is not a callable", type);
        dispose_disposable_obj(func, func_disposable);
        raiseException(ObjectNotCallableException(buffer));
    }

    // handles stack overflow error
    if (stack_ptr >= MAX_STACK_SIZE - 1 && func->data.Func->functype == REGULAR_FUNC)
    {
        const char *funcname = func->data.Func->func_data.user_func.func_name;
        char buffer[strlen(funcname) + 50];
        snprintf(buffer, sizeof(buffer), "Stack Overflow Error when calling function '%s'", funcname);
        dispose_disposable_obj(func, func_disposable);
        raiseException(StackOverflowException(buffer));
    }

    CallFrame *new_frame = NULL;

    switch (func->data.Func->functype)
    {
    case EXCEPTION_CONSTRUCTOR_FUNC:
    {
        const char *funcname = func->data.Func->func_data.exception_constructor.exception_name;
        if (arg_count > 1)
        {
            char buffer[110 + strlen(funcname)];
            snprintf(buffer, sizeof(buffer),
                     "%s Exception Constructor can only take 1 or 0 arguments, but was given %zu",
                     funcname, arg_count);

            dispose_disposable_obj(func, func_disposable);
            raiseException(InvalidNumberOfArgumentsException(buffer));
        }

        RtObject *exception_obj = init_RtObject(EXCEPTION_TYPE);
        exception_obj->data.Exception = init_RtException(funcname, NULL);
        exception_obj->data.Exception->msg =
            arg_count == 1 ? rtobj_toString(arguments[0]) : cpy_string("");

        StackMachine_push(stk_machine, exception_obj, true);
        break;
    }
    case BUILTIN_FUNC:
    {

        assert(func->data.Func->func_data.built_in.func);
        assert(!Intermediate_raisedException);

        BuiltinFunc *builtin = func->data.Func->func_data.built_in.func;
        RtObject *obj = builtin->builtin_func((RtObject **)arguments, arg_count);

        // if an exception occured during execution of built in function
        if (Intermediate_raisedException)
        {
            assert(!obj);
            dispose_disposable_obj(func, func_disposable);
            raiseException(Intermediate_raisedException);
        }
        assert(obj);

        // pushes result of function onto stack machine
        StackMachine_push(StackMachine, obj, true);
        break;
    }

    case ATTR_BUILTIN_FUNC:
    {
        assert(!Intermediate_raisedException);

        AttrBuiltin *attrfunc = func->data.Func->func_data.attr_built_in.func;
        RtObject *target = func->data.Func->func_data.attr_built_in.target;
        RtObject *obj = attrfunc->func.builtin_func(target, arguments, arg_count);

        // exception occurred during execution of built in attribute function
        if (Intermediate_raisedException)
        {
            assert(!obj);
            dispose_disposable_obj(func, func_disposable);
            raiseException(Intermediate_raisedException);
        }
        assert(obj);

        StackMachine_push(StackMachine, obj, obj != target);
        break;
    }

    case REGULAR_FUNC:
        new_frame =
            perform_regular_func_call(func, func_disposable, arguments, arg_disposable, arg_count);
        break;
    }

    dispose_disposable_obj(func, func_disposable);
    return new_frame;
}

/**
 * DESCRIPTION:
 * Helper for performing logic for handling regular function calls
 */
static CallFrame *perform_regular_func_call(
    RtObject *funcobj,
    bool funcobj_disposable,
    RtObject **arguments,
    bool disposable[],
    size_t arg_count)
{
    assert(funcobj);
    assert(funcobj->type == FUNCTION_TYPE);
    assert(funcobj->data.Func->functype == REGULAR_FUNC);

    RtFunction *func = funcobj->data.Func;

    ByteCodeList *func_code = func->func_data.user_func.body;
    char *funcname = func->func_data.user_func.func_name;
    char *func_file_location = func->func_data.user_func.file_location;

    // checks argument counts match
    if (func->functype == REGULAR_FUNC &&
        func->func_data.user_func.arg_count != INT64_MAX &&
        func->func_data.user_func.arg_count != arg_count)
    {
        size_t expected_arg_count = func->func_data.user_func.arg_count;

        char buffer[110 + strlen(funcname)];
        snprintf(
            buffer,
            sizeof(buffer),
            "'%s': Function expected %zu arguments, but got %zu\n",
            funcname ? funcname : "(Unknown)",
            expected_arg_count,
            arg_count);

        dispose_disposable_obj(funcobj, funcobj_disposable);

        raiseException(InvalidNumberOfArgumentsException(buffer));
    }

    CallFrame *new_frame =
        init_CallFrame(func_code, func, func_file_location);

    // adds function arguments to lookup table and ref list
    // arguments are shallowed copied if there of a primitive type, and added into GC
    // if an object is disposable, then no copy is needed
    for (unsigned int i = 0; i < arg_count; i++)
    {
        RtObject *arg = arguments[i];
        char *argname = func->func_data.user_func.args[i];

        if (!disposable[i])
            assert(GC_Registry_has(arg));

        Identifier_Table_add_var(new_frame->lookup, argname, arg, DOES_NOT_APPLY);
    }

    // adds closure variables
    for (unsigned int i = 0; i < func->func_data.user_func.closure_count; i++)
    {
        char *closure_name = func->func_data.user_func.closures[i];
        RtObject *closure_obj = func->func_data.user_func.closure_obj[i];
        // rtobj_refcount_increment1(closure_obj);
        Identifier_Table_add_var(new_frame->lookup, closure_name, closure_obj, DOES_NOT_APPLY);
    }

    // adds function definition to lookup (to allow recursion)
    // thats why we perform a shallow copy
    if (funcname)
    {
        RtObject *cpy_func = init_RtObject(FUNCTION_TYPE);
        cpy_func->data.Func = rtfunc_cpy(func, true);

        Identifier_Table_add_var(new_frame->lookup, funcname, cpy_func, DOES_NOT_APPLY);

        // call frame is pushed before adding func to GC registry
        add_to_GC_registry(cpy_func);
    }

    RunTime_push_callframe(new_frame);
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

        obj = rtobj_rt_preprocess(obj, disposable, false);
        // if object is not disposable, then a shallow copy is created
        // since its not disposable, it means its already in the GC registry, so it can be discarded
        // tmp[i] = disposable ? obj : rtobj_shallow_cpy(obj);
        addDisposablePrimitiveToGC(disposable, obj);
        tmp[i] = obj;
    }

    RtList *list = init_RtList(length >= DEFAULT_RTLIST_LEN ? length * 2 : DEFAULT_RTLIST_LEN);
    for (int i = (int)length - 1; i >= 0; i--)
    {
        rtlist_append(list, tmp[i]);
    }

    listobj->data.List = list;
    StackMachine_push(StackMachine, listobj, true);

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

        val = rtobj_rt_preprocess(val, valdispose, false);
        key = rtobj_rt_preprocess(key, valdispose, false);
        rtmap_insert(map, key, val);

        addDisposablePrimitiveToGC(valdispose, val);
        addDisposablePrimitiveToGC(keydispose, key);
    }
    RtObject *mapobj = init_RtObject(HASHMAP_TYPE);
    mapobj->data.Map = map;

    StackMachine_push(StackMachine, mapobj, true);
}

/**
 * DESCRIPTION:
 * Creates a runtime set by popping some amount of objects from the stack machine
 */
static void perform_create_set(int setsize)
{

    RtSet *set = init_RtSet(setsize + 1);

    for (int i = 0; i < setsize; i++)
    {
        bool dispose = disposable();
        RtObject *obj = StackMachine_pop(StackMachine, false);
        assert(obj);
        obj = rtobj_rt_preprocess(obj, dispose, false);
        rtset_insert(set, obj);

        addDisposablePrimitiveToGC(dispose, obj);
        // add_to_GC_registry(obj);
    }

    RtObject *setobj = init_RtObject(HASHSET_TYPE);
    setobj->data.Set = set;

    StackMachine_push(StackMachine, setobj, true);
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

    assert(!Intermediate_raisedException);

    RtObject *indexed_obj = rtobj_getindex(obj, index_);

    dispose_disposable_obj(index_, index_disposable);
    dispose_disposable_obj(obj, obj_disposable);

    // if exception occurred during indexing
    if (Intermediate_raisedException)
    {
        assert(!indexed_obj);
        raiseException(Intermediate_raisedException);
        return;
    }

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

    RtClass *cl =
        init_RtClass(getCurrentStackFrame()->function->func_data.user_func.func_name);
    cl->body = getCurrentStackFrame()->function;

    if (!cl)
        MallocError();

    for (unsigned int i = 0; fields[i] != NULL; i++)
    {
        if (fields[i]->access != PUBLIC_ACCESS)
            continue;

        RtObject *attrname = init_RtObject(STRING_TYPE);
        attrname->data.String = init_RtString(fields[i]->key);

        rtmap_insert(cl->attrs_table, attrname, fields[i]->obj);

        add_to_GC_registry(attrname);
        add_to_GC_registry(fields[i]->obj);
    }

    free(fields);

    RtObject *class = init_RtObject(CLASS_TYPE);
    class->data.Class = cl;

    StackMachine_push(StackMachine, class, true);
}
/******************************************************/

/**
 * Performs a logic for creating variables
 */
static void perform_create_var(char *varname, AccessModifier access)
{
    bool disposable = disposable();
    RtObject *new_val = StackMachine_pop(StackMachine, false);
    CallFrame *frame = getCurrentStackFrame();

    RtObject *cpy;
    // = rtobj_rt_preprocess(new_val, disposable);
    if (disposable)
    {
        cpy = new_val;
    }
    else
    {
        assert(GC_Registry_has(new_val));
        if (rttype_isprimitive(new_val->type))
            cpy = rtobj_deep_cpy(new_val, false);
        else
            cpy = rtobj_shallow_cpy(new_val);
    }

    Identifier_Table_add_var(frame->lookup, varname, cpy, access);

    add_to_GC_registry(cpy);
    // addDisposablePrimitiveToGC(disposable, cpy);
}

/**
 * DESCRIPTION:
 * Logic for creating a variable
 */
static void perform_create_exception(const char *exception_name, AccessModifier access)
{
    CallFrame *frame = getCurrentStackFrame();
    RtObject *obj = init_RtObject(FUNCTION_TYPE);
    obj->data.Func = init_rtfunc(EXCEPTION_CONSTRUCTOR_FUNC);
    obj->data.Func->func_data.exception_constructor.exception_name = cpy_string(exception_name);

    Identifier_Table_add_var(frame->lookup, exception_name, obj, access);

    add_to_GC_registry(obj);
}

/**
 * DESCRIPTION:
 * Contains logic for getting atribute from a rt object
 */
static void perform_get_attribute(const char *attrs)
{
    bool target_disposable = disposable();
    RtObject *target = StackMachine_pop(StackMachine, false);

    // handles special cases

    if (target->type == NULL_TYPE)
    {
        char buffer[60 + strlen(attrs)];
        snprintf(buffer, sizeof(buffer), "Attemped to fetch attribute '%s' on Null type.", attrs);
        dispose_disposable_obj(target, target_disposable);
        raiseException(NullTypeException(buffer)) return;
    }

    if (target->type == UNDEFINED_TYPE)
    {
        char buffer[60 + strlen(attrs)];
        snprintf(buffer, sizeof(buffer), "Attemped to fetch attribute '%s' on Undefined type.", attrs);
        dispose_disposable_obj(target, target_disposable);
        raiseException(UndefinedTypeException(buffer)) return;
    }

    if (target->type == CLASS_TYPE)
    {
        RtObject *attrsname = init_RtObject(STRING_TYPE);
        attrsname->data.String = init_RtString(attrs);
        RtObject *attrs = rtmap_get(target->data.Class->attrs_table, attrsname);

        if (attrs)
        {
            StackMachine_push(StackMachine, attrs, false);

            rtobj_free(attrsname, false, true);
            dispose_disposable_obj(target, target_disposable);
            return;
        }

        rtobj_free(attrsname, false, true);
    }

    RtObject *builtin_attr = rtattr_getattr(target, attrs);

    // if builtin attribute does not exist
    if (!builtin_attr)
    {
        RtException *exc = init_InvalidAttrsException(target, attrs);
        dispose_disposable_obj(target, target_disposable);
        raiseException(exc);
        return;
    }

    if (target_disposable)
        assert(!GC_Registry_has(target));
    else
        assert(GC_Registry_has(target));

    add_to_GC_registry(target);

    StackMachine_push(StackMachine, builtin_attr, true);
}

/**
 * DESCRIPTION:
 * Handles logic for byte code instruction RAISE_EXCEPTION_IF_COMPARE_EXCEPTION_FALSE
 */
static void perform_raise_exception_if_compare_exception_false()
{
    assert(raisedException);

    bool disposable = disposable();
    RtObject *obj = StackMachine_pop(stk_machine, false);

    if (obj->type != EXCEPTION_TYPE)
    {
        RtException *exc = init_InvalidRaiseTypeException(obj);
        dispose_disposable_obj(obj, disposable);
        raiseException(exc);
    }

    // currently raised Exception does not match the pattern matched exception
    if (!rtexception_compare(raisedException, obj->data.Exception))
    {
        dispose_disposable_obj(obj, disposable);
        raiseException(raisedException);
    }

    dispose_disposable_obj(obj, disposable);
}

/**
 * DESCRIPTION:
 * Performs logic for OFFSET_JUMP_IF_COMPARE_EXCEPTION_FALSE
 */
static void perform_offset_jump_if_compare_exception_false(int offset)
{
    bool disposable = disposable();
    RtObject *obj = StackMachine_pop(stk_machine, false);

    // handles case where catch block does receive exception type
    if (obj->type != EXCEPTION_TYPE)
    {
        RtException *exc = init_InvalidRaiseTypeException(obj);
        dispose_disposable_obj(obj, disposable);
        raiseException(exc);
        return;
    }

    if (!rtexception_compare(raisedException, obj->data.Exception))
    {
        getCurrentStackFrame()->pg_counter += offset;
    }

    dispose_disposable_obj(obj, disposable);
}

/**
 * DESCRIPTION:
 * Raises exception
 */
static void perform_raise_exception()
{
    bool disposable = disposable();
    RtObject *obj = StackMachine_pop(stk_machine, false);

    if (obj->type != EXCEPTION_TYPE)
    {
        RtException *exc = init_InvalidRaiseTypeException(obj);
        dispose_disposable_obj(obj, disposable);
        raiseException(exc);
        return;
    }

    RtException *exception = rtexception_cpy(obj->data.Exception);
    dispose_disposable_obj(obj, disposable);
    handle_runtime_exception(exception);
}

int run_program()
{
    while (true)
    {
        CallFrame *frame = callStack[stack_ptr];
        ByteCodeList *bytecode = frame->pg;

        // used for exception handling
        int new_pg_counter = setjmp((int *)frame->exception_jump);
        frame = callStack[stack_ptr];
        bytecode = frame->pg;
        if (new_pg_counter != 0)
        {
            frame->pg_counter = new_pg_counter;
        }

        while (true)
        {
            ByteCode *code = bytecode->code[frame->pg_counter];
            bool loop = true;

            switch (code->op_code)
            {
            case LOAD_CONST:
            {
                // contants are imbedded within the bytecode
                // therefore a deep copy is created
                StackMachine_push(StackMachine,
                                  rtobj_deep_cpy(code->data.LOAD_CONST.constant, false), true);
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
                assert(!Intermediate_raisedException);
                if (!logical_not_op(StackMachine->head->obj))
                {
                    assert(Intermediate_raisedException);
                    raiseException(Intermediate_raisedException);
                }
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

            case ABSOLUTE_JUMP:
            {
                frame->pg_counter = code->data.ABSOLUTE_JUMP.offset;
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

            case LOAD_ATTRIBUTE:
            {
                perform_get_attribute(code->data.LOAD_ATTR.attribute_name);
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

            case CREATE_SET:
            {
                perform_create_set(code->data.CREATE_SET.set_size);
                break;
            }

            case LOAD_INDEX:
            {
                perform_get_index();
                break;
            }
            case PUSH_EXCEPTION_HANDLER:
            {
                push_exception_handler(
                    stack_ptr,
                    frame->pg_counter + code->data.PUSH_EXCEPTION_HANDLER.start_of_catch_block,
                    stk_machine->size);
                break;
            }

            case POP_EXCEPTION_HANDLER:
            {
                free_exception_handler(pop_exception_handler());
                break;
            }

            case CREATE_EXCEPTION:
            {
                perform_create_exception(
                    code->data.CREATE_EXCEPTION.exception,
                    code->data.CREATE_EXCEPTION.access);
                break;
            }

            case RAISE_EXCEPTION_IF_COMPARE_EXCEPTION_FALSE:
            {
                perform_raise_exception_if_compare_exception_false();
                break;
            }

            case OFFSET_JUMP_IF_COMPARE_EXCEPTION_FALSE:
            {
                perform_offset_jump_if_compare_exception_false(
                    code->data.OFFSET_JUMP_IF_COMPARE_EXCEPTION_FALSE.offset);
                break;
            }

            case RAISE_EXCEPTION:
            {
                perform_raise_exception();
                break;
            }

            case RESOLVE_RAISED_EXCEPTION:
            {
                assert(raisedException);
                rtexception_free(raisedException);
                raisedException = NULL;
                break;
            }
            case EXIT_PROGRAM:
            {
                return perform_exit();
            }
            }

            trigger_GC();

            if (!loop)
                break;

            frame->pg_counter++;
        }
    }
    return 0;
}
