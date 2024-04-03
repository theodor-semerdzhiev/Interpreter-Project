#pragma once
#include <stdbool.h>
#include "rttype.h"
#include "../rtlib/builtinfuncs.h"
#include "rtexception.h"
#include "rtlists.h"
#include "rtfunc.h"
#include "rtmap.h"
#include "rtset.h"
#include "rtclass.h"
#include "rtstring.h"
#include "rtnumber.h"

// Forward declaration
typedef struct RtObject RtObject;
typedef struct ByteCodeList ByteCodeList;
typedef struct RtList RtList;
typedef struct RtMap RtMap;
typedef struct RtSet RtSet;
typedef struct Identifier Identifier;
typedef struct RtClass RtClass;
typedef struct RtException RtException;
typedef enum RtType RtType;

// Generic object for all variables
typedef struct RtObject
{
    RtType type;

    union
    {
        size_t *GCrefcount_NULL_TYPE;

        size_t *GCrefcount_UNDEFINED_TYPE;

        RtNumber *Number;

        RtString *String;

        RtFunction *Func;

        RtList *List;

        RtMap *Map;

        RtClass *Class;

        RtSet *Set;

        RtException *Exception;
    } data;
} RtObject;

RtObject *init_RtObject(RtType type);

RtObject *rtobj_rt_preprocess(RtObject *obj, bool disposable, bool add_to_GC);

char *rtobj_toString(const RtObject *obj);
RtObject *multiply_objs(RtObject *obj1, RtObject *obj2);

RtObject *add_objs(RtObject *obj1, RtObject *obj2);
RtObject *substract_objs(RtObject *obj1, RtObject *obj2);
RtObject *divide_objs(RtObject *obj1, RtObject *obj2);
RtObject *modulus_objs(RtObject *obj1, RtObject *obj2);
RtObject *exponentiate_obj(RtObject *base, RtObject *exponent);
RtObject *bitwise_and_objs(RtObject *obj1, RtObject *obj2);
RtObject *bitwise_or_objs(RtObject *obj1, RtObject *obj2);
RtObject *bitwise_xor_objs(RtObject *obj1, RtObject *obj2);
RtObject *shift_left_objs(RtObject *obj1, RtObject *obj2);
RtObject *shift_right_objs(RtObject *obj1, RtObject *obj2);
RtObject *greater_than_op(RtObject *obj1, RtObject *obj2);
RtObject *greater_equal_op(RtObject *obj1, RtObject *obj2);
RtObject *lesser_than_op(RtObject *obj1, RtObject *obj2);
RtObject *lesser_equal_op(RtObject *obj1, RtObject *obj2);
RtObject *equal_op(RtObject *obj1, RtObject *obj2);
RtObject *logical_and_op(RtObject *obj1, RtObject *obj2);
RtObject *logical_or_op(RtObject *obj1, RtObject *obj2);
RtObject *logical_not_op(RtObject *target);

bool eval_obj(const RtObject *obj);
void rtobj_init_cmp_tbl();
int rtobj_compare(const RtObject *obj1, const RtObject *obj2);

void rtobj_print(const RtObject *obj);

unsigned int rtobj_hash(const RtObject *obj);
unsigned int rtobj_hash_data_ptr(const RtObject *obj);
bool rtobj_shallow_equal(const RtObject *obj1, const RtObject *obj2);
bool rtobj_equal(const RtObject *obj1, const RtObject *obj2);

RtObject *rtobj_shallow_cpy(const RtObject *obj);
RtObject *rtobj_deep_cpy(const RtObject *obj, bool add_to_gc);

RtObject *rtobj_mutate(RtObject *target, const RtObject *new_value, bool new_val_disposable);
RtObject *rtobj_getindex(const RtObject *obj, const RtObject *index);

RtObject **rtobj_getrefs(const RtObject *obj);
void *rtobj_getdata(const RtObject *obj);

size_t rtobj_refcount(const RtObject *obj);
size_t rtobj_increment_refcount(RtObject *obj, size_t n);
size_t rtobj_decrement_refcount(RtObject *obj, size_t n);

/* A few macro */
#define rtobj_refcount_increment1(obj) rtobj_increment_refcount(obj, 1);
#define rtobj_refcount_decrement1(obj) rtobj_decrement_refcount(obj, 1);

void rtobj_free_data(RtObject *obj, bool free_immutable, bool update_ref_counts);
void rtobj_free(RtObject *obj, bool free_immutable, bool update_ref_counts);
void rtobj_shallow_free(RtObject *obj);
void rtobj_deconstruct(RtObject *obj, int offset);