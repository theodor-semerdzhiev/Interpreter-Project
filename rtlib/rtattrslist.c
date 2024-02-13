#include <assert.h>
#include "rtattrs.h"
#include "../generics/hashmap.h"

static RtObject *builtin_list_append(RtObject *list, RtObject **args, int argcount);
static RtObject *builtin_list_pop(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_list_popLast(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_list_popFirst(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_list_clear(RtObject *obj, RtObject **args, int argcount);

static const AttrBuiltinKey _list_append_key = {LIST_TYPE, "append"};
static const AttrBuiltin _list_append = 
{LIST_TYPE, {.builtin_func = builtin_list_append}, -1, "append", true};

static const AttrBuiltinKey _list_pop_key = {LIST_TYPE, "pop"};
static const AttrBuiltin _list_pop = 
{LIST_TYPE, {.builtin_func = builtin_list_pop}, -1, "pop", true};

static const AttrBuiltinKey _list_popLast_key = {LIST_TYPE, "popLast"};
static const AttrBuiltin _list_popLast = 
{LIST_TYPE, {.builtin_func = builtin_list_popLast}, -1, "popLast", true};

static const AttrBuiltinKey _list_popFirst_key = {LIST_TYPE, "popFirst"};
static const AttrBuiltin _list_popFirst = 
{LIST_TYPE, {.builtin_func = builtin_list_popFirst}, -1, "popFirst", true};

static const AttrBuiltinKey _list_clear_key = {LIST_TYPE, "clear"};
static const AttrBuiltin _list_clear = 
{LIST_TYPE, {.builtin_func = builtin_list_clear}, -1, "clear", true};


void init_RtListAttr(GenericMap *registry) {
    addToAttrRegistry(registry, _list_append_key, _list_append);
    addToAttrRegistry(registry, _list_pop_key, _list_pop);
    addToAttrRegistry(registry, _list_popLast_key, _list_popLast);
    addToAttrRegistry(registry, _list_popFirst_key, _list_popFirst);
    addToAttrRegistry(registry, _list_clear_key, _list_clear);
}

/**
 * DESCRIPTION:
 * Builtin function for adding objects to a rt list
 */
static RtObject *builtin_list_append(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount > 0); // temporary
    assert(obj->type == LIST_TYPE);
    RtList *list = obj->data.List;
    for (int i = 0; i < argcount; i++)
    {
        rtlist_append(list, args[i]);
    }
    return obj;
}

/**
 * DESCRIPTION:
 * Builtin function for removing objects from a rt list, at a specific index
 */
static RtObject *builtin_list_pop(RtObject *obj, RtObject **args, int argcount)
{
    // temporary
    assert(argcount  == 1); 
    assert(obj->type == LIST_TYPE);
    assert(args[0]->type == NUMBER_TYPE);

    RtList *list = obj->data.List;
    double number = args[0]->data.Number->number;

    rtlist_remove(list, (size_t)number);
    return obj;
}

/**
 * DESCRIPTION:
 * Built in function for adding a element into a specific index of a rt list
*/
static RtObject *builtin_list_popLast(RtObject *obj, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 0); 
    assert(obj->type == LIST_TYPE);
    rtlist_poplast(obj->data.List);
    return obj;
}

/**
 * DESCRIPTION:
 * Built in function for adding a element into a specific index of a rt list
*/
static RtObject *builtin_list_popFirst(RtObject *obj, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 0); 
    assert(obj->type == LIST_TYPE);
    rtlist_popfirst(obj->data.List);
    return obj;
}

/**
 * DESCRIPTION:
 * Built in function for adding a element into a specific index of a rt list
*/
static RtObject *builtin_list_clear(RtObject *obj, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 0); 
    assert(obj->type == LIST_TYPE);

    while(obj->data.List->length > 0) {
        rtlist_remove(obj->data.List, obj->data.List->length-1);
    }

    assert(obj->data.List->length == 0);
    return obj;
}
