#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
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
static unsigned long GC_THRESHOLD = 3;

static unsigned long liveObjCount = 0;
static bool gc_active = false;
static GenericSet *GCregistry = NULL;

bool is_GC_Active() { return gc_active; }

typedef struct RtDataCtn {
    void *rtdata;
    RtType type;
} RtDataCtn;

/**
 * DESCRIPTION:
 * Creates Runtime data container for keeping track of data pointers
 * 
 * PARAMS:
 * rtdata: pointer to store
 * type: its type
*/
static RtDataCtn *init_RtDataCtn(void *rtdata, RtType type) {
    RtDataCtn *ctn = malloc(sizeof(RtDataCtn));
    if(!ctn) {
        MallocError();
        return NULL;
    }
    ctn->rtdata=rtdata;
    ctn->type=type;
    return ctn;
}

/**
 * DESCRIPTION:
 * Frees rtdata container
*/
static void rtdatactn_free(RtDataCtn *ctn) {
    rttype_freedata(ctn->type, ctn, false);
    free(ctn);
}

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

    void *data = rtobj_getdata(obj);
    if(!data) 
        return obj;

    RtDataCtn *ctn = init_RtDataCtn(data, obj->type);
    if (GenericSet_get(GCregistry, ctn)) {
        free(ctn);
        return obj;
    }

    GenericSet_insert(GCregistry, ctn, false);
    liveObjCount++;
    return obj;
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
        GC_THRESHOLD *= 2;
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
 * free_ptr: wether object should be freed when removed
 */
void remove_from_GC_registry(RtObject *obj, bool free_ptr)
{
    assert(obj);
    if(rttype_isprimitive(obj->type))
        return;
    void *data = rtobj_getdata(obj);
    if(!data) 
        return;

    RtDataCtn *ptr = (RtDataCtn *)GenericSet_remove(GCregistry, data);
    if (ptr)
        liveObjCount--;
    if (free_ptr)
        rtdatactn_free(ptr);
    
    return free_ptr ? NULL : ptr;
}

/**
 * DESCRIPTION:
 * These functions will be used in the GC registry, to compare/hash references to RtObjects (i.e raw pointers).
 * Since we are comparing raw pointers, we are going to be hashing that raw value
 */
static bool _compare_rawptr(const void *ptr1, const void *ptr2) { return ptr1 == ptr2; }
static unsigned int _hash_rtdata(const RtDataCtn *ptr) { return hash_pointer(ptr->rtdata); }
static void _free_rtdata(RtDataCtn *ptr) { rtdatactn_free(ptr); }

/* Initializes the runtime garbage collector */
void init_GarbageCollector()
{
    // TODO
    gc_active = true;
    GCregistry = init_GenericSet(
        _compare_rawptr,
        (unsigned int(*)(const void*))_hash_rtdata,
        (void (*)(void *))rtdatactn_free);

    if(!GCregistry) {
        printf("Failed to initialize GC registry\n");
        MallocError();
    }
}

void cleanup_GarbageCollector()
{
    liveObjCount = 0;
    gc_active = false;
    GenericSet_free(GCregistry, true);
    GCregistry = NULL;
}

static void traverse_reference_graph(RtObject *root);

/**
 * DESCRIPTION:
 * This is a Mark and Sweep algorithm, responsible for performing garbage collection
 * 1- Gets all active objs from the GC registry
 * 2- Unmarks all live objects
 * 3- Starting from the root Call Frame, and it takes each variable in its lookup table, marks it,
 * Then performs a DFS on its reference graph, marking all objects that are reachable from original root
 * 4- All objects that were not marked, could not be reached, therefor that memory is freed
 *
 */
void garbageCollect()
{

    CallFrame **callStack = getCallStack();

    // performs a DFS on all live objects in lookup tables
    for (int i = 0; i <= getCallStackPointer(); i++)
    {
        RtObject **elements = IdentifierTable_to_list(callStack[i]->lookup);
        for (int j = 0; elements[j] != NULL; j++)
        {
            traverse_reference_graph(elements[j]);
        }
        free(elements);
    }

    // performs a DFS on the Stack Machine
    RtObject **stk_machine_list = StackMachine_to_list(getCurrentStkMachineInstance());
    unsigned int stkmachine_size = getCurrentStkMachineInstance()->size;
    for (unsigned int i = 0; i < stkmachine_size; i++)
    {
        traverse_reference_graph(stk_machine_list[i]);
    }

    RtObject **all_active_obj = (RtObject **)GenericSet_to_list(GCregistry);

    // Frees all unreachable nodes
    for (unsigned long i = 0; i < liveObjCount; i++)
    {
        if (!all_active_obj[i]->mark)
        {
            RtObject *tmp = remove_from_GC_registry(all_active_obj[i], true);
        }
    }

    free(all_active_obj);
    all_active_obj = (RtObject **)GenericSet_to_list(GCregistry);

    // resets mark flags
    for (unsigned long i = 0; i < liveObjCount; i++)
    {
        all_active_obj[i]->mark = false;
    }
    for (unsigned long i = 0; i < stkmachine_size; i++)
    {
        stk_machine_list[i]->mark = false;
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
    if (root->mark)
        return;

    root->mark = true;

    RtObject **refs = rtobj_getrefs(root);
    for (unsigned int i = 0; refs[i] != NULL; i++)
    {
        traverse_reference_graph(refs[i]);
    }

    free(refs);
}
