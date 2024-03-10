#include <stdlib.h>
#include <assert.h>
#include "runtime.h"
#include "rtexception.h"
#include "rtexchandler.h"

RtException *raisedException = NULL;

static RtExceptionHandler *head = NULL;
static RtExceptionHandler *tail = NULL;

#define HasExceptionHandler() head == NULL &&tail == NULL

#define initExceptionHandler() malloc(sizeof(RtExceptionHandler));


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
 * 1- Pops all call frames until it gets to the proper one
 * 2- Pops Stack Machine until its reached the state that it was when the try block was invoked
 * 3- Long jumps to the run_program() and starts running the try block
 */
void handle_runtime_exception(RtException *exception)
{
    if (tail == NULL && head == NULL)
        // TODO
        assert(false);

    RtExceptionHandler *handler = pop_exception_handler();
    assert(handler);

    for (size_t i = 0; i < getCallStackPointer() - handler->stack_ptr; i++)
    {
        free_CallFrame(RunTime_pop_callframe(), false);
    }

    StackMachine *stkmachine = getCurrentStkMachineInstance();

    for (size_t i = 0; i < getCurrentStkMachineInstance()->size - handler->stk_machine_ptr; i++)
    {
        StackMachine_pop(stkmachine, true);
    }

    CallFrame *currentframe = getCurrentStackFrame();
    int new_pg_counter = handler->start_of_try_catch;

    free_exception_handler(handler);
    raisedException = exception;
    longjmp((int *)currentframe->exception_jump, new_pg_counter);
}
