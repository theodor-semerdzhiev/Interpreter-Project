#include <assert.h>
#include <string.h>
#include "rtattrs.h"
#include "../generics/hashmap.h"
#include "../runtime/rtexchandler.h"

static RtObject *builtin_set_add(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_remove(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_contains(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_clear(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_tolist(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_union(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_intersection(RtObject *target, RtObject **args, int argcount);


static const AttrBuiltinKey _str_uppercase_key = {HASHSET_TYPE, "add"};
static const AttrBuiltin _set_add =
    {HASHSET_TYPE, {.builtin_func = builtin_set_add}, 2, "add", true};

static const AttrBuiltinKey _set_remove_key = {HASHSET_TYPE, "remove"};
static const AttrBuiltin _set_remove =
    {HASHSET_TYPE, {.builtin_func = builtin_set_remove}, 1, "remove", true};

static const AttrBuiltinKey _set_contains_key = {HASHSET_TYPE, "contains"};
static const AttrBuiltin _set_contains =
    {HASHSET_TYPE, {.builtin_func = builtin_set_contains}, 1, "contains", true};

static const AttrBuiltinKey _set_clear_key = {HASHSET_TYPE, "clear"};
static const AttrBuiltin _set_clear =
    {HASHSET_TYPE, {.builtin_func = builtin_set_clear}, 1, "clear", true};

static const AttrBuiltinKey _set_tolist_key = {HASHSET_TYPE, "toList"};
static const AttrBuiltin _set_tolist =
    {HASHSET_TYPE, {.builtin_func = builtin_set_tolist}, 1, "toList", true};

static const AttrBuiltinKey _set_union_key = {HASHSET_TYPE, "union"};
static const AttrBuiltin _set_union =
    {HASHSET_TYPE, {.builtin_func = builtin_set_union}, 1, "union", true};

static const AttrBuiltinKey _set_intersection_key = {HASHSET_TYPE, "intersection"};
static const AttrBuiltin _set_intersection =
    {HASHSET_TYPE, {.builtin_func = builtin_set_intersection}, 1, "intersection", true};

// Helper for abstracting away invalid number of arguments exception
#define setInvalidNumberOfArgsIntermediateException(map_attr, actual_args, expected_args) \
    setIntermediateException(init_InvalidNumberOfArgumentsException( \
        "Set attribute function " map_attr, (size_t)actual_args, (size_t)expected_args))

// Helper for abstracting away Invalid argument type exception
#define setInvalidArgTypeException(attr_name, expected_type, argobj) \
    setIntermediateException(init_InvalidTypeException_Builtin("attribute " attr_name, expected_type, argobj))

/**
 * DESCRIPTION:
 * Inserts built in functions for sets into the registry 
*/
void init_RtSetAttr(GenericMap *registry)
{
    addToAttrRegistry(registry, _str_uppercase_key, _set_add);
    addToAttrRegistry(registry, _set_remove_key, _set_remove);
    addToAttrRegistry(registry, _set_contains_key, _set_contains);
    addToAttrRegistry(registry, _set_clear_key, _set_clear);
    addToAttrRegistry(registry, _set_tolist_key, _set_tolist);
    addToAttrRegistry(registry, _set_union_key, _set_union);
    addToAttrRegistry(registry, _set_intersection_key, _set_intersection);
}

/**
 * DESCRIPTION:
 * Inserts an element into a runtime set
 */
static RtObject *builtin_set_add(RtObject *target, RtObject **args, int argcount)
{
    assert(target->type == HASHSET_TYPE);

    if(argcount != 1) {
        setInvalidNumberOfArgsIntermediateException("add()", argcount, 1);
        return NULL;
    }

    RtSet *set = target->data.Set;
    RtObject *val = args[0];

    // you cannot add set to itself
    if(target == val || (val->type == HASHSET_TYPE && val->data.Set == set)) {
        char* settostr = rtobj_toString(val);
        char buffer[80 + strlen(settostr)];
        snprintf(buffer, sizeof(buffer), 
        "Cannot add Set Object %s to itself. Sets cannot contain themselves.", settostr);
        free(settostr);
        setIntermediateException(InvalidTypeException(buffer));
        return NULL;
    }

    rtset_insert(set, val);
    return target;
}

/**
 * DESCRIPTION:
 * Removes element from set
 */
static RtObject *builtin_set_remove(RtObject *target, RtObject **args, int argcount)
{
    assert(target->type == HASHSET_TYPE);

    if(argcount != 1) {
        setInvalidNumberOfArgsIntermediateException("remove()", argcount, 1);
        return NULL;
    }

    RtSet *set = target->data.Set;
    RtObject *val = args[0];
    rtset_remove(set, val);
    return target;
}

/**
 * DESCRIPTION:
 * Checks if a set contains an element
 */
static RtObject *builtin_set_contains(RtObject *target, RtObject **args, int argcount)
{
    assert(target->type == HASHSET_TYPE);

    if(argcount != 1) {
        setInvalidNumberOfArgsIntermediateException("contains()", argcount, 1);
        return NULL;
    }

    RtSet *set = target->data.Set;
    RtObject *val = args[0];
    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(rtset_get(set, val) != NULL);
    return res;
}

/**
 * DESCRIPTION:
 * Clears a set
 */
static RtObject *builtin_set_clear(RtObject *target, RtObject **args, int argcount)
{
    assert(target->type == HASHSET_TYPE);

    if(argcount != 1) {
        setInvalidNumberOfArgsIntermediateException("clear()", argcount, 1);
        return NULL;
    }

    (void)args;
    RtSet *set = target->data.Set;
    rtset_clear(set, false, false);
    return target;
}

/**
 * DESCRIPTION:
 * Converts a set into a list
 */
static RtObject *builtin_set_tolist(RtObject *target, RtObject **args, int argcount)
{
    assert(target->type == HASHSET_TYPE);

    if(argcount != 0) {
        setInvalidNumberOfArgsIntermediateException("toList()", argcount, 1);
        return NULL;
    }

    (void)args;
    RtSet *set = target->data.Set;
    RtList *list = init_RtList(set->size);
    RtObject **contents = rtset_getrefs(set);
    for (size_t i = 0; contents[i] != NULL; i++)
        rtlist_append(list, contents[i]);
    free(contents);
    RtObject *listobj = init_RtObject(LIST_TYPE);
    listobj->data.List = list;
    return listobj;
}

/**
 * DESCRIPTION:
 * Builtin function for performing union set operation 
*/
static RtObject *builtin_set_union(RtObject *target, RtObject **args, int argcount) {
    assert(target->type == HASHSET_TYPE);

    if(argcount != 1) {
        setInvalidNumberOfArgsIntermediateException("union()", argcount, 1);
        return NULL;
    }

    if(args[0]->type != HASHSET_TYPE) {
        setInvalidArgTypeException("union()", "Set", args[0]);
        return NULL;
    }

    RtSet *new_set = rtset_union(target->data.Set, args[0]->data.Set, true, true);
    RtObject *obj = init_RtObject(HASHSET_TYPE);
    obj->data.Set = new_set;
    return obj;
}

/**
 * DESCRIPTION:
 * Builtin function for performing intersection set operation
*/
static RtObject *builtin_set_intersection(RtObject *target, RtObject **args, int argcount) {
    assert(target->type == HASHSET_TYPE);

    if(argcount != 1) {
        setInvalidNumberOfArgsIntermediateException("intersection()", argcount, 1);
        return NULL;
    }
    
    if(args[0]->type != HASHSET_TYPE) {
        setInvalidArgTypeException("intersection()", "Set", args[0]);
        return NULL;
    }

    RtSet *new_set = rtset_intersection(target->data.Set, args[0]->data.Set, true, true);
    RtObject *obj = init_RtObject(HASHSET_TYPE);
    obj->data.Set = new_set;
    return obj;

}