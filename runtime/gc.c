#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "rtobjects.h"
#include "runtime.h"
#include "../generics/hashset.h"
#include "../generics/utilities.h"
#include "gc.h"
#include "rttype.h"

/**
 * This is temporary file in order to implement the large scale refactor of the GC
 */
/**
 * This File contains the implementation of the runtime garbage collector
 */

/* When the number of active objects reached this amount, garbage collector performs a rotation     */
static unsigned long GC_THRESHOLD = 100;
static unsigned long liveObjCount = 0;

static unsigned long ticks_since_last_collection = 0;

static bool gc_active = false;
static GenericSet *GCregistry = NULL;
static GenericSet *freed_ptrs_set = NULL;

bool is_GC_Active() { return gc_active; }

// Macro for 
#define InitPtrSet(free_func) init_GenericSet(ptr_equal, hash_pointer, free_func);

/**
 * DESCRIPTION:
 * This function adds the object to the Garbage Collector registry.
 * If GC is not active, then function returns early
 * This function will always return the input object
 *
 * PARAMS:
 * obj: object to add
 * trigger_collection: wether garbage collection should get triggered if GC threshold is reached
 */
RtObject *add_to_GC_registry(RtObject *obj)
{
    assert(obj);
    if (!gc_active)
        return obj;

    if (GenericSet_has(GCregistry, obj))
    {
        return obj;
    }

    GenericSet_insert(GCregistry, obj, false);
    liveObjCount++;

    assert(GCregistry->size == liveObjCount);
    return obj;
}

/**
 * DESCRIPTION:
 * Checks if obj is contained within the GC registry
*/
bool GC_Registry_has(const RtObject *obj) {
    return GenericSet_has(GCregistry, obj);
}

/**
 * DESCRIPTION:
 * Triggers garbage collection if GC threshold as been reached
 */
void trigger_GC()
{
    if (liveObjCount >= GC_THRESHOLD)
    {
        garbageCollect();
        GC_THRESHOLD = liveObjCount * 10;
        ticks_since_last_collection = 0;
    } else {
        ticks_since_last_collection++;
    }
}

/**
 * DESCRIPTION:
 * Removes object from GC registry, if free_ptr is true, object will be freed and NULL will be returned
 * If pointer is not in registry, NULL will be returned
 * Otherwise, the object from the set will be returned
 *
 * PARAMS:
 * obj: object to remove from GC registry
 * free_rtobj: wether rt object should be freed when data associated is removed
 */
RtObject *remove_from_GC_registry(RtObject *obj, bool free_rtobj)
{
    assert(obj);

    RtObject *ptr = (RtObject *)GenericSet_remove(GCregistry, obj);
    if (!ptr)
    {
        if (free_rtobj)
            rtobj_free(obj, false);
        return free_rtobj ? NULL : obj;
    }

    liveObjCount--;

    if (free_rtobj)
    {
        rtobj_free(obj, false);
    }

    return free_rtobj ? NULL : obj;
}

/**
 * DESCRIPTION:
 * These functions will be used in the GC registry, to compare/hash references to RtObjects (i.e raw pointers).
 * Since we are comparing raw pointers, we are going to be hashing that raw value
 */
static bool _compare_rtdatactn(const RtObject *ptr1, const RtObject *ptr2)
{
    return ptr1 == ptr2;
}

static unsigned int _hash_rtobjptr(const RtObject *ptr)
{
    return hash_pointer(ptr);
}

static void _free_rtobj(RtObject *ptr)
{
    rtobj_free(ptr, false);
}

/**
 * DESCRIPTION:
 * Initializes the runtime garbage collector
 * */
void init_GarbageCollector()
{
    // TODO
    gc_active = true;
    GCregistry = init_GenericSet(
        (bool (*)(const void *, const void *))_compare_rtdatactn,
        (unsigned int (*)(const void *))_hash_rtobjptr,
        (void (*)(void *))_free_rtobj);

    if (!GCregistry)
    {
        printf("Failed to initialize GC registry\n");
        MallocError();
    }
}


/**
 * DESCRIPTION:
 * This function contains all logic for freeing and disposing of the GC registry permanently
 */
static void cleanup_GCRegistry()
{
    GenericSet *set = InitPtrSet(NULL);
    if (!set) {
        printf("Memory Allocation Error occurred during GC Registry cleanup");
        MallocError();
    }

    RtObject **objs = (RtObject**)GenericSet_to_list(GCregistry);
    GenericSet_free(GCregistry, false);
    GenericSet_free(freed_ptrs_set, false);

    // marks all unique pointers to free
    for (unsigned long i = 0; objs[i] != NULL; i++) {
        void *data = rtobj_getdata(objs[i]);
        if(GenericSet_has(set, data)) {
            rtobj_shallow_free(objs[i]);
        } else {
            GenericSet_insert(set, data, false);
            rtobj_free(objs[i], false);
        }
    }

    free(objs);
    GenericSet_free(set, false);
}

/**
 * DESCRIPTION:
 * Cleanups Garbage Collector memory
 */
void cleanup_GarbageCollector()
{
    gc_active = false;
    cleanup_GCRegistry();
    liveObjCount = 0;
    GCregistry = NULL;
    freed_ptrs_set = NULL;
}

static void traverse_reference_graph(RtObject *root);

/**
 * DESCRIPTION:
 * This is a Mark and Sweep algorithm, responsible for performing garbage collection
 * 1- Traverses all objects stored in the call frames, for each object visited, its marked
 * 2- Traverses all objects still on the stack machine, for each object visited, its marked
 * NOTE: For all objects that are visited, a DFS is performed on its reference graph to visit all reachable objects
 * 3- For all objects in the GCRegistry, if its not marked, that it means its unreachable, and its subsequently freed
 * 4- GC flags for all objects in the call frames and stack machine is reset to false
 * NOTE: In order to prevent double free errors, a set is used to keep track of data pointers that are freed
 */
void garbageCollect()
{

    CallFrame **callStack = getCallStack();

    // performs a DFS on all live objects in lookup tables
    for (int i = getCallStackPointer(); i >= 0 ; i--)
    {
        RtObject **elements = IdentifierTable_to_list(callStack[i]->lookup);
        for (int j = 0; elements[j] != NULL; j++)
            traverse_reference_graph(elements[j]);

        free(elements);
    }

    // performs a DFS on the Stack Machine
    RtObject **stk_machine_list = StackMachine_to_list(getCurrentStkMachineInstance());
    for (unsigned int i = 0; stk_machine_list[i] != NULL; i++)
        traverse_reference_graph(stk_machine_list[i]);

    RtObject **all_active_obj = (RtObject **)GenericSet_to_list(GCregistry);

    if(!freed_ptrs_set)
        freed_ptrs_set = InitPtrSet(NULL);

    // Frees all unreachable nodes
    for (unsigned long i = 0; i < liveObjCount; i++)
    {
        void *data = rtobj_getdata(all_active_obj[i]);
        assert(data);

        if(GenericSet_has(freed_ptrs_set, data)) {
            rtobj_shallow_free(all_active_obj[i]);
            continue;
        }

        if (!rttype_get_GCFlag(data, all_active_obj[i]->type) ) {
            GenericSet_insert(freed_ptrs_set, data, false);
            RtObject *obj = remove_from_GC_registry(all_active_obj[i], false);
            assert(obj);
            rtobj_free(obj, false);
        } else {
            rtobj_set_GCFlag(all_active_obj[i], false);
        }
    }
    
    GenericSet_clear(freed_ptrs_set, false);

    // resets mark flags in the stack machine
    for (unsigned long i = 0; stk_machine_list[i] != NULL; i++)
        rtobj_set_GCFlag(stk_machine_list[i], false);


    // resets mark flags
    for (int i = 0; i <= getCallStackPointer(); i++)
    {
        RtObject **elements = IdentifierTable_to_list(callStack[i]->lookup);
        for (int j = 0; elements[j] != NULL; j++)
            rtobj_set_GCFlag(elements[j], false);

        free(elements);
    }

    free(all_active_obj);
    free(stk_machine_list);
}

/**
 * DESCRIPTION:
 * This object performs a DFS traversal of the reference graph, and marks all reachable nodes
 */
static void traverse_reference_graph(RtObject *root)
{
    if (rtobj_get_GCFlag(root))
        return;

    rtobj_set_GCFlag(root, true);

    RtObject **refs = rtobj_getrefs(root);
    for (unsigned int i = 0; refs[i] != NULL; i++)
    {
        traverse_reference_graph(refs[i]);
    }

    free(refs);
}
