#include <assert.h>
#include "rtattrs.h"
#include "../generics/hashmap.h"

static RtObject *builtin_set_add(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_remove(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_contains(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_clear(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_set_tolist(RtObject *target, RtObject **args, int argcount);

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
}

/**
 * DESCRIPTION:
 * Inserts an element into a runtime set
 */
static RtObject *builtin_set_add(RtObject *target, RtObject **args, int argcount)
{
    // temporary
    assert(argcount == 1);
    assert(target->type == HASHSET_TYPE);
    RtSet *set = target->data.Set;
    RtObject *val = args[0];
    rtset_insert(set, val);
    return target;
}

/**
 * DESCRIPTION:
 * Removes element from set
 */
static RtObject *builtin_set_remove(RtObject *target, RtObject **args, int argcount)
{
    // temporary
    assert(argcount == 1);
    assert(target->type == HASHSET_TYPE);
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
    // temporary
    assert(argcount == 1);
    assert(target->type == HASHSET_TYPE);
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
    // temporary
    assert(argcount == 0);
    assert(target->type == HASHSET_TYPE);
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
    // temporary
    assert(argcount == 0);
    assert(target->type == HASHSET_TYPE);
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