#include <assert.h>
#include "rtattrs.h"
#include "../runtime/gc.h"
#include "../generics/hashmap.h"

static RtObject *builtin_map_add(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_remove(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_containskey(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_contains_val(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_clear(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_getkeys(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_getvalues(RtObject *obj, RtObject **args, int argcount);
static RtObject *builtin_map_getitems(RtObject *obj, RtObject **args, int argcount);


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

static const AttrBuiltinKey _map_keys_key = {HASHMAP_TYPE, "keys"};
static const AttrBuiltin _map_keys =
    {HASHMAP_TYPE, {.builtin_func = builtin_map_getkeys}, 1, "keys", true};

static const AttrBuiltinKey _map_values_key = {HASHMAP_TYPE, "values"};
static const AttrBuiltin _map_values =
    {HASHMAP_TYPE, {.builtin_func = builtin_map_getvalues}, 1, "values", true};

static const AttrBuiltinKey _map_items_key = {HASHMAP_TYPE, "items"};
static const AttrBuiltin _map_items =
    {HASHMAP_TYPE, {.builtin_func = builtin_map_getitems}, 1, "items", true};

/**
 * DESCRIPTION:
 * Inserts RtMap attribute functions to the registry
 */
void init_RtMapAttr(GenericMap *registry)
{
    addToAttrRegistry(registry, _map_add_key, _map_add);
    addToAttrRegistry(registry, _map_remove_key, _map_remove);
    addToAttrRegistry(registry, _map_containsKey_key, _map_containsKey);
    addToAttrRegistry(registry, _map_containsVal_key, _map_containsVal);
    addToAttrRegistry(registry, _map_clear_key, _map_clear);
    addToAttrRegistry(registry, _map_keys_key, _map_keys);
    addToAttrRegistry(registry, _map_values_key, _map_values);
    addToAttrRegistry(registry, _map_items_key, _map_items);
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

    for (int i = 0; contents[i] != NULL; i++)
    {
        if (rtobj_equal(val, contents[i]))
        {
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
    rtmap_clear(map, false, false, false);
    return obj;
}

/**
 * DESCRIPTION:
 * Builtin function for getting all keys in a map
 */
static RtObject *builtin_map_getkeys(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount == 0); // temporary
    assert(obj->type == HASHMAP_TYPE);
    (void)args;

    RtMap *map = obj->data.Map;
    RtObject **keys = rtmap_getrefs(map, true, false);
    RtList *list = init_RtList(map->size);
    
    for(size_t i=0; keys[i] != NULL; i++) 
        rtlist_append(list, keys[i]);

    free(keys);

    RtObject *newlist = init_RtObject(LIST_TYPE);
    newlist->data.List = list;
    return newlist;
}

/**
 * DESCRIPTION:
 * Builtin function for getting all value in a map
 */
static RtObject *builtin_map_getvalues(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount == 0); // temporary
    assert(obj->type == HASHMAP_TYPE);
    (void)args;

    RtMap *map = obj->data.Map;
    RtObject **values = rtmap_getrefs(map, false, true);
    RtList *list = init_RtList(map->size);
    
    for(size_t i=0; values[i] != NULL; i++) 
        rtlist_append(list, values[i]);
        
    free(values);

    RtObject *newlist = init_RtObject(LIST_TYPE);
    newlist->data.List = list;
    return newlist;
}

/**
 * DESCRIPTION:
 * Creates a list of 2 size lists representing each key value pair
*/
static RtObject *builtin_map_getitems(RtObject *obj, RtObject **args, int argcount)
{
    assert(argcount == 0); // temporary
    assert(obj->type == HASHMAP_TYPE);
    (void)args;

    RtMap *map = obj->data.Map;
    RtObject **keyval = rtmap_getrefs(map, true, true);
    RtList *list = init_RtList(map->size);
    
    for(size_t i=0; keyval[i] != NULL;) {
        RtList *pair = init_RtList(2);
        rtlist_append(pair, keyval[i++]);
        rtlist_append(pair, keyval[i++]);

        RtObject *obj = init_RtObject(LIST_TYPE);
        obj->data.List = pair;
        add_to_GC_registry(obj);
        rtlist_append(list, obj); 
    }
        
    free(keyval);

    RtObject *newlist = init_RtObject(LIST_TYPE);
    newlist->data.List = list;
    return newlist;
}
