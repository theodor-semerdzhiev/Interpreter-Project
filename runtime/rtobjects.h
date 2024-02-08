#pragma once
#include <stdbool.h>
#include "builtins.h"
#include "rtlists.h"
#include "rtfunc.h"
#include "rtmap.h"
#include "rtclass.h"
#include "rtstring.h"
#include "rtnumber.h"
#include "rttype.h"

// Forward declaration
typedef struct RtObject RtObject;
typedef struct ByteCodeList ByteCodeList;
typedef struct RtList RtList;
typedef struct RtMap RtMap;
typedef struct Identifier Identifier;
typedef struct RtClass RtClass;


// Generic object for all variables
typedef struct RtObject
{
    RtType type;

    union
    {
        bool *GCFlag_NULL_TYPE; // represents the GC flag for the null type, MUST be malloced

        bool *GCFlag_UNDEFINED_TYPE; // represents the undefined type GC flag, MUST be malloced

        RtNumber *Number;

        RtString *String;

        RtFunction *Func;

        RtList *List;

        RtMap *Map;

        RtClass *Class;
        // HashSet *set;
    } data;
} RtObject;

RtObject *init_RtObject(RtType type);

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
unsigned int rtobj_hash(const RtObject *obj);
unsigned int rtobj_hash_data_ptr(const RtObject *obj);
bool rtobj_shallow_equal(const RtObject *obj1, const RtObject *obj2);
bool rtobj_equal(const RtObject *obj1, const RtObject *obj2);

RtObject *rtobj_shallow_cpy(const RtObject *obj);
RtObject *rtobj_deep_cpy(const RtObject *obj);

RtObject *rtobj_mutate(RtObject *target, const RtObject *new_value, bool new_val_disposable);
RtObject *rtobj_getindex(RtObject *obj, RtObject *index);

RtObject **rtobj_getrefs(const RtObject *obj);
void *rtobj_getdata(const RtObject *obj);
bool rtobj_get_GCFlag(const RtObject *obj);
void rtobj_set_GCFlag(RtObject *obj, bool flag);
RtObject *rtobj_init(RtType type, void* data);

void rtobj_free_data(RtObject *obj, bool free_immutable);
void rtobj_free(RtObject *obj, bool free_immutable);
void rtobj_shallow_free(RtObject *obj);
void rtobj_deconstruct(RtObject *obj, int offset);