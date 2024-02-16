#include <assert.h>
#include "rtattrs.h"
#include "../generics/hashmap.h"


static RtObject *builtin_map_add(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_remove(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_containskey(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_contains_val(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_clear(RtObject *obj, RtObject **args, int argcount);

static const AttrBuiltinKey _map_add_key = {HASHMAP_TYPE, "add"};
static const AttrBuiltin _map_add =
    {HASHMAP_TYPE, {.builtin_func = builtin_map_add}, 2, "add", true};

static const AttrBuiltinKey _map_remove_key = {HASHMAP_TYPE, "remove"};
static const AttrBuiltin _map_remove =
    {HASHMAP_TYPE, {.builtin_func = builtin_map_remove}, 1, "remove", true};

static const AttrBuiltinKey _map_containsKey_key = {HASHMAP_TYPE, "containsKey"};
static const AttrBuiltin _map_containsKey =
    {HASHMAP_TYPE, {.builtin_func = builtin_map_containskey}, 1, "containsKey", true};

static const AttrBuiltinKey _map_containsVal_key = {HASHMAP_TYPE, "containsVal"};
static const AttrBuiltin _map_containsVal =
    {HASHMAP_TYPE, {.builtin_func = builtin_map_contains_val}, 1, "containsVal", true};

static const AttrBuiltinKey _map_clear_key = {HASHMAP_TYPE, "clear"};
static const AttrBuiltin _map_clear =
    {HASHMAP_TYPE, {.builtin_func = builtin_map_clear}, 1, "clear", true};

/**
 * DESCRIPTION:
 * Inserts RtMap attribute functions to the registry
*/
void init_RtMapAttr(GenericMap *registry) {
    addToAttrRegistry(registry, _map_add_key, _map_add);
    addToAttrRegistry(registry, _map_remove_key, _map_remove);
    addToAttrRegistry(registry, _map_containsKey_key, _map_containsKey);
    addToAttrRegistry(registry, _map_containsVal_key, _map_containsVal);
    addToAttrRegistry(registry, _map_clear_key, _map_clear);
}


/**
 * DESCRIPTION:
 * Builtin function for adding key value pairs to a map
 */
static RtObject *builtin_map_add(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount == 2); // temporary
    assert(obj->type == HASHMAP_TYPE);

    RtMap *map = obj->data.Map;
    RtObject *key = args[0];
    RtObject *val = args[1];
    rtmap_insert(map, key, val);
    return obj;
}


/**
 * DESCRIPTION:
 * Builtin function for removing key value pairs from a map
 */
static RtObject *builtin_map_remove(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount == 1); // temporary
    assert(obj->type == HASHMAP_TYPE);

    RtMap *map = obj->data.Map;
    RtObject *key = args[0];
    rtmap_remove(map, key);
    return obj;
}

/**
 * DESCRIPTION:
 * Builtin function for checking if a key is contained within a map
 */
static RtObject *builtin_map_containskey(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount == 1); // temporary
    assert(obj->type == HASHMAP_TYPE);

    RtMap *map = obj->data.Map;
    RtObject *key = args[0];
    
    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(rtmap_get(map, key) != NULL);
    return res;
}

/**
 * DESCRIPTION:
 * Builtin function for checking if a key is contained within a map
 */
static RtObject *builtin_map_contains_val(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount == 1); // temporary
    assert(obj->type == HASHMAP_TYPE);

    RtMap *map = obj->data.Map;
    RtObject *val = args[0];
    
    bool contains = false;
    RtObject **contents = rtmap_getrefs(map, false, true);

    for(int i=0; contents[i] != NULL; i++) {
        if(rtobj_equal(val, contents[i])) {
            contains = true;
            break;
        }
    }
    free(contents);

    RtObject *res = init_RtObject(NUMBER_TYPE);
    res->data.Number = init_RtNumber(contains);
    return res;
}


/**
 * DESCRIPTION:
 * Builtin function for clearing a map
 */
static RtObject *builtin_map_clear(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount == 0); // temporary
    assert(obj->type == HASHMAP_TYPE);
    (void)args;

    RtMap *map = obj->data.Map;
    rtmap_clear(map, false, false ,false);
    return obj;
}