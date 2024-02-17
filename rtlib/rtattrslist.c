#include <assert.h>
#include "rtattrs.h"
#include "../generics/hashmap.h"
#include "../generics/utilities.h"

static RtObject *builtin_list_append(RtObject *list, RtObject **args, int argcount);
static RtObject *builtin_list_pop(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_list_popLast(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_list_popFirst(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_list_clear(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_list_contains(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_list_remove(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_list_toset(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_list_sort(RtObject *target, RtObject **args, int argcount);
static RtObject *builtin_list_reverse(RtObject *target, RtObject **args, int argcount);

static const AttrBuiltinKey _list_append_key = {LIST_TYPE, "append"};
static const AttrBuiltin _list_append = 
{LIST_TYPE, {.builtin_func = builtin_list_append}, -1, "append", true};

static const AttrBuiltinKey _list_pop_key = {LIST_TYPE, "pop"};
static const AttrBuiltin _list_pop = 
{LIST_TYPE, {.builtin_func = builtin_list_pop}, 1, "pop", true};

static const AttrBuiltinKey _list_popLast_key = {LIST_TYPE, "popLast"};
static const AttrBuiltin _list_popLast = 
{LIST_TYPE, {.builtin_func = builtin_list_popLast}, 0, "popLast", true};

static const AttrBuiltinKey _list_popFirst_key = {LIST_TYPE, "popFirst"};
static const AttrBuiltin _list_popFirst = 
{LIST_TYPE, {.builtin_func = builtin_list_popFirst}, 0, "popFirst", true};

static const AttrBuiltinKey _list_clear_key = {LIST_TYPE, "clear"};
static const AttrBuiltin _list_clear = 
{LIST_TYPE, {.builtin_func = builtin_list_clear}, 0, "clear", true};

static const AttrBuiltinKey _list_contains_key = {LIST_TYPE, "contains"};
static const AttrBuiltin _list_contains = 
{LIST_TYPE, {.builtin_func = builtin_list_contains}, 1, "contains", true};

static const AttrBuiltinKey _list_remove_key = {LIST_TYPE, "remove"};
static const AttrBuiltin _list_remove = 
{LIST_TYPE, {.builtin_func = builtin_list_remove}, -1, "remove", true};

static const AttrBuiltinKey _list_toset_key = {LIST_TYPE, "toSet"};
static const AttrBuiltin _list_toset = 
{LIST_TYPE, {.builtin_func = builtin_list_toset}, -1, "toSet", true};

static const AttrBuiltinKey _list_sort_key = {LIST_TYPE, "sort"};
static const AttrBuiltin _list_sort = 
{LIST_TYPE, {.builtin_func = builtin_list_sort}, -1, "sort", true};

static const AttrBuiltinKey _list_reverse_key = {LIST_TYPE, "reverse"};
static const AttrBuiltin _list_reverse = 
{LIST_TYPE, {.builtin_func = builtin_list_reverse}, -1, "reverse", true};

void init_RtListAttr(GenericMap *registry) {
    addToAttrRegistry(registry, _list_append_key, _list_append);
    addToAttrRegistry(registry, _list_pop_key, _list_pop);
    addToAttrRegistry(registry, _list_popLast_key, _list_popLast);
    addToAttrRegistry(registry, _list_popFirst_key, _list_popFirst);
    addToAttrRegistry(registry, _list_clear_key, _list_clear);
    addToAttrRegistry(registry, _list_contains_key, _list_contains);
    addToAttrRegistry(registry, _list_remove_key, _list_remove);
    addToAttrRegistry(registry, _list_toset_key, _list_toset);
    addToAttrRegistry(registry, _list_sort_key, _list_sort);
    addToAttrRegistry(registry, _list_reverse_key, _list_reverse);
}

/**
 * DESCRIPTION:
 * Builtin function for adding objects to a rt list
 */
static RtObject *builtin_list_append(RtObject *target, RtObject **args, int argcount)
{
    assert(target->type == LIST_TYPE);
    RtList *list = target->data.List;
    for (int i = 0; i < argcount; i++)
    {
        rtlist_append(list, args[i]);
    }
    return target;
}

/**
 * DESCRIPTION:
 * Builtin function for removing objects from a rt list, at a specific index
 */
static RtObject *builtin_list_pop(RtObject *target, RtObject **args, int argcount)
{
    // temporary
    assert(argcount  == 1); 
    assert(target->type == LIST_TYPE);
    assert(args[0]->type == NUMBER_TYPE);

    RtList *list = target->data.List;
    double number = args[0]->data.Number->number;

    RtObject *removed = rtlist_removeindex(list, (size_t)number);
    if(!removed) {
        printf("Cannot removed index out of range\n");
    }
    return target;
}

/**
 * DESCRIPTION:
 * Built in function for adding a element into a specific index of a rt list
*/
static RtObject *builtin_list_popLast(RtObject *target, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 0); 
    assert(target->type == LIST_TYPE);
    (void)args;
    rtlist_poplast(target->data.List);
    return target;
}

/**
 * DESCRIPTION:
 * Built in function for adding a element into a specific index of a rt list
*/
static RtObject *builtin_list_popFirst(RtObject *target, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 0); 
    assert(target->type == LIST_TYPE);
    (void)args;
    rtlist_popfirst(target->data.List);
    return target;
}

/**
 * DESCRIPTION:
 * Built in function for adding a element into a specific index of a rt list
*/
static RtObject *builtin_list_clear(RtObject *target, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 0); 
    assert(target->type == LIST_TYPE);
    (void)args;

    while(target->data.List->length > 0) {
        rtlist_removeindex(target->data.List, target->data.List->length-1);
    }

    assert(target->data.List->length == 0);
    return target;
}

/**
 * DESCRIPTION:
 * Built in function for checking if an element is found in a list
*/
static RtObject *builtin_list_contains(RtObject *target, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 1); 
    assert(target->type == LIST_TYPE);
    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(rtlist_contains(target->data.List, args[0]));
    return res;
}

/**
 * DESCRIPTION:
 * Built in function for removing some amount of matching rt objects in a list
*/
static RtObject *builtin_list_remove(RtObject *target, RtObject **args, int argcount) {
    // temporary
    assert(target->type == LIST_TYPE);
    for(int i = 0; i < argcount; i++) {
        rtlist_remove(target->data.List, args[i]);
    }
    return target;
}

/**
 * DESCRIPTION:
 * Built in function for converting a list to a set
*/
static RtObject *builtin_list_toset(RtObject *target, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 0);
    assert(target->type == LIST_TYPE);
    (void)args;

    RtList *list = target->data.List;
    RtSet* set = init_RtSet(list->length);

    for(size_t i=0; i < list->length; i++) {
        rtset_insert(set, list->objs[i]);
    }

    RtObject *setobj = init_RtObject(HASHSET_TYPE);
    setobj->data.Set = set;
    return setobj;
}

/**
 * DESCRIPTION:
 * Built in function for reversing a list
*/
static RtObject *builtin_list_reverse(RtObject *target, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 0);
    assert(target->type == LIST_TYPE);
    (void)args;
    rtlist_reverse(target->data.List);
    return target;
}   

static int _rtobj_compare_wrapper(const RtObject **o1, const RtObject **o2);
static int _rtobj_reverse_compare_wrapper(const RtObject **o1, const RtObject **o2);

/**
 * Useful macro for sorting lists (in regular or reverse order)
*/
#define SortList(rtlistobj, reverse) \
    qsort(rtlistobj->data.List->objs, \ 
    rtlistobj->data.List->length, \
    sizeof(*(rtlistobj->data.List->objs)), \
    (int (*)(const void *, const void *)) \
    (reverse? _rtobj_reverse_compare_wrapper :_rtobj_compare_wrapper)); 

/**
 * DESCRIPTION:
 * Helper function for sorting in regular order, used by the builtin sort function
 * This function allows us to also sort other lists recursively
*/
static int _rtobj_compare_wrapper(const RtObject **o1, const RtObject **o2) {
    int res = rtobj_compare(*o1, *o2);
    if((*o1)->type == LIST_TYPE) 
        SortList((*o1), false);
    if((*o2)->type == LIST_TYPE) 
        SortList((*o2), false);
    return res;
}

/**
 * DESCRIPTION:
 * Helper function for sorting in reverse, used by the builtin sort function
 * This function allows us to also sort other lists recursively
*/
static int _rtobj_reverse_compare_wrapper(const RtObject **o1, const RtObject **o2) {
    int res = -rtobj_compare(*o1, *o2);
    if((*o1)->type == LIST_TYPE) 
        SortList((*o1), true);
    if((*o2)->type == LIST_TYPE) 
        SortList((*o2), true);
    return res;
}

/**
 * DESCRIPTION:
 * Built in function for sorting a list in place
 * 
 * This function can take a single parameter, if the param = "reverse", 
 * then the list will be sorted in reverse order
*/
static RtObject *builtin_list_sort(RtObject *target, RtObject **args, int argcount) {
    // temporary
    assert(argcount == 1 || argcount == 0);
    assert(target->type == LIST_TYPE);
    (void)args;

    bool reverse = false;
    if( argcount == 1 && 
        args[0]->type == STRING_TYPE &&
        strings_equal(args[0]->data.String->string, "reverse")) {
        
        reverse = true;
    }

    SortList(target, reverse);
    
    return target;
}


