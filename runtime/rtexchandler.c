#include <stdlib.h>
#include <assert.h>
#include "../main.h"
#include "runtime.h"
#include "../generics/utilities.h"
#include "rtexception.h"
#include "rtexchandler.h"

/**
 * This variable stores the currently raised exception
 * When the exception is resolved, memory is freed, and the closure is reset to NULL
 * 
 * NOTE:
 * This variable should NEVER be contained within the GC, its always independant
*/
RtException *raisedException = NULL;

/**
 * This variable is responsible for storing intermediate exceptions created during runtime operations
 * When an exception is raised, this variable gets reset
*/
RtException *Intermediate_raisedException = NULL;

static RtExceptionHandler *head = NULL;
static RtExceptionHandler *tail = NULL;

#define HasExceptionHandler() head == NULL &&tail == NULL

#define initExceptionHandler() malloc(sizeof(RtExceptionHandler));

/**
 * DESCRIPTION:
 * Useful helper for setting raised Exception and making sure certain conditions are met
*/
void _set_raised_exception(RtException *exc) {
    assert(exc);
    assert(!raisedException);
    raisedException = exc;
}

/**
 * DESCRIPTION:
 * Useful helper for setting intermidate exception, and checks predicates
*/
void _set_intermediate_exception(RtException *exc) {
    assert(exc);
    assert(!Intermediate_raisedException);
    Intermediate_raisedException = exc;
}

/**
 * DESCRIPTION:
 * Frees entire exception handler linked list
 */
void free_exception_handlers()
{

    RtExceptionHandler *ptr = head;
    while (ptr)
    {
        RtExceptionHandler *tmp = ptr->next;
        free_exception_handler(ptr)
        ptr = tmp;
    }
    head = NULL;
    tail = NULL;
}

/**
 * DESCRIPTION:
 * Pushes an exception handler on the link list
 */
RtExceptionHandler *push_exception_handler(size_t stack_ptr, size_t start_of_try_catch, size_t stk_machine_ptr)
{
    if (!head)
    {
        head = initExceptionHandler();
        head->prev = NULL;
        head->next = NULL;
        head->stack_ptr = stack_ptr;
        head->start_of_try_catch = start_of_try_catch;
        head->stk_machine_ptr = stk_machine_ptr;
        tail = head;
    }
    else
    {
        tail->next = initExceptionHandler();
        tail->next->stack_ptr = stack_ptr;
        tail->next->start_of_try_catch = start_of_try_catch;
        tail->next->stk_machine_ptr = stk_machine_ptr;
        tail->next->prev = tail;
        tail->next->next = NULL;
        tail = tail->next;
    }
    return tail;
}

/**
 * DESCRIPTION:
 * Pops an exception from stack
 */
RtExceptionHandler *pop_exception_handler()
{
    assert(head);
    RtExceptionHandler *popped = tail;
    tail = tail->prev;
    if (!tail)
        head = NULL;
    else
        tail->next = NULL;
    return popped;
}

/**
 * DESCRIPTION:
 * Contains logic for properly handling exception
 * 
 * PREDICATE:
 * exception paramter MUST NOT be contained in the GC, 
 * it should be able to freed at anytime without any problem
 *
 * 1- Pops all call frames until it gets to the proper one
 * 2- Pops Stack Machine until its reached the state that it was when the try block was invoked
 * 3- Long jumps to the run_program() and starts running the try block
 */
void handle_runtime_exception(RtException *exception)
{
    assert(exception);


    // resets Intermediate variable
    if(exception == Intermediate_raisedException) {
        Intermediate_raisedException = NULL;
    }
    
    assert(!Intermediate_raisedException);
    
    // if there is not exception handler, or there is already another raised Exception
    if ((tail == NULL && head == NULL) || raisedException) {
        print_unhandledexception(exception);
        
        // handles special case
        if(exception != raisedException) {
            rtexception_free(exception);
            rtexception_free(raisedException);
        } else {
            rtexception_free(raisedException);
        }

        raisedException = NULL;
        free_exception_handlers();
        
        longjmp(global_program_interrupt, 1);
        return;
    }

    raisedException = exception;

    RtExceptionHandler *handler = pop_exception_handler();
    assert(handler);

    // pops until we reach the Call Frame with the catch block
    for (size_t i = 0; i < getCallStackPointer() - handler->stack_ptr; i++)
    {
        free_CallFrame(RunTime_pop_callframe(), false);
    }

    // Resets the stk machine to the original state when the try block was started 
    StackMachine *stkmachine = getCurrentStkMachineInstance();

    for (size_t i = 0; i < getCurrentStkMachineInstance()->size - handler->stk_machine_ptr; i++)
    {
        StackMachine_pop(stkmachine, true);
    }

    // sets the updated to pg counter to the long jump 
    CallFrame *currentframe = getCurrentStackFrame();
    int new_pg_counter = handler->start_of_try_catch;

    free_exception_handler(handler);
    longjmp((int *)currentframe->exception_jump, new_pg_counter);
}

/**
 * DESCRIPION:
 * Functions prints out unhandled exception error
*/
#define TAB2 "      "
#define TAB4 "              "
#define StackPtr getCallStackPointer()

#define RED_TEXT "\x1b[31m"
#define RESET_COLOR "\x1b[0m"
#define PRINT_STACK_LIMIT 30

void print_unhandledexception(RtException *exception) {
    assert(exception);
    printf("\n");

    // Error message will depend on the currently raised exception
    if(raisedException) {
        printf(RED_TEXT "Unhandled exception '%s' occured while trying to resolve exception %s \n", exception->ex_name, raisedException->ex_name);
    } else {
        printf(RED_TEXT "Unhandled exception '%s' occured: \n", exception->ex_name);
    }

    printf("Message: %s\n" "Call Stack: (MAX %zu)\n", exception->msg, MAX_CALLSTACK_SIZE);

    CallFrame *frame;
    long calldepth = StackPtr;

    while(StackPtr >= 0)
    {
        int count = 0;
        if(StackPtr > 0) {
            frame = RunTime_pop_callframe();
            assert(frame && frame->function);
            size_t cur_linenb = frame->pg->code[frame->pg_counter]->line_nb;
            char *functostring = rtfunc_toString(frame->function);

            if((calldepth - StackPtr) <= PRINT_STACK_LIMIT || StackPtr <= PRINT_STACK_LIMIT) {
                printf("%ld    %s:%zu" TAB2 "%s()" TAB2 "Function Signature: %s\n", 
                    StackPtr + 1,
                    frame->code_file_location, 
                    cur_linenb,
                    rtfunc_get_funcname(frame->function),
                    functostring);

                free(functostring);
            } else if((calldepth - StackPtr) == PRINT_STACK_LIMIT + 1) {
                printf(TAB4 ".\n" TAB4 ".\n" TAB4 ".\n");
                printf("    (%ld more Call Frames ...)\n", StackPtr + 1 - PRINT_STACK_LIMIT);
                printf(TAB4 ".\n" TAB4 ".\n" TAB4 ".\n");
            }
            count++;
        } else {
            frame = RunTime_pop_callframe();
            assert(!frame->function);
            size_t cur_linenb = frame->pg->code[frame->pg_counter]->line_nb;
            printf("%ld    %s:%zu\n", 
                StackPtr + 1, 
                frame->code_file_location, 
                cur_linenb);
        }

        free_CallFrame(frame, false);
    }
    printf(RESET_COLOR);
}

