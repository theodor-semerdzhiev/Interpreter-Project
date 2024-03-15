#pragma once
#include "rtexception.h"

typedef struct RtExceptionHandler RtExceptionHandler;
/**
 * DESCRIPTION:
 * This struct represents a exception handler
 *
 * stack_ptr: the current call depth
 * start_of_try_block: the absolute index of the try catch chain
 * stk_machine_ptr: the current size of the stack machine when entering the try block
 */
typedef struct RtExceptionHandler
{
    size_t stack_ptr;
    size_t start_of_try_catch;
    size_t stk_machine_ptr;
    RtExceptionHandler *next;
    RtExceptionHandler *prev;
} RtExceptionHandler;

extern RtException *raisedException;

#define free_exception_handler(handler) free(handler);

void _set_raised_exception(RtException *exc);

RtExceptionHandler *push_exception_handler(size_t stack_ptr, size_t start_of_try_catch, size_t stk_machine_ptr);
RtExceptionHandler *pop_exception_handler();
void handle_runtime_exception(RtException *exception);
void print_unhandledexception(RtException *exception);

#define setRaisedException(exception) _set_raised_exception(exception);

#define raiseException(exception) handle_runtime_exception(exception);