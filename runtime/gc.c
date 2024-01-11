#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "rtobjects.h"
#include "runtime.h"
#include "../generics/hashset.h"
#include "../generics/utilities.h"
#include "gc.h"


/**
 * This File contains the implementation of the runtime garbage collector
*/

/* When the number of active objects reached this amount, garbage collector performs a rotation     */
static unsigned long GC_THRESHOLD = 10;


static unsigned long liveObjCount = 0;
static bool gc_active = false;
static GenericSet *GCregistry = NULL;


bool is_GC_Active() {return gc_active;}

/**
 * DESCRIPTION:
 * This function adds the object to the Garbage Collector registry.
 * If GC is not active, then function returns early
 * This function will always return the input object
 * 
 * PARAMS:
 * obj: object to add
*/
RtObject *add_to_GC_registry(RtObject *obj) {
    if(!gc_active) return obj;
    if(GenericSet_get(GCregistry, obj)) return obj;

    GenericSet_insert(GCregistry, obj, false);
    liveObjCount++;

    // if(liveObjCount >= GC_THRESHOLD) {
    //     garbageCollect();
    //     GC_THRESHOLD *=2;
    // }
    return obj;
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

RtObject *remove_from_GC_registry(RtObject *obj, bool free_ptr) {
    // assert(GenericSet_get(GCregistry, obj));
    RtObject *ptr = (RtObject*) GenericSet_remove(GCregistry, obj);
    if(free_ptr)
        free_RtObject(ptr, false);
    liveObjCount--;
    
    return free_ptr? NULL: ptr;
}


/**
 * DESCRIPTION:
 * These functions will be used in the GC registry, to compare/hash references to RtObjects (i.e raw pointers).
 * Since we are comparing raw pointers, we are going to be hashing that raw value
*/
static bool _compare_rawptr(const void *ptr1, const void *ptr2) {return ptr1 == ptr2;}
static unsigned int _hash_RtObject_ptr(const void *ptr) {return hash_pointer(ptr);}
static void _free_RtObject(RtObject *obj) {free_RtObject(obj, false);}

/* Initializes the runtime garbage collector */
void init_GarbageCollector() {
    // TODO
    gc_active = true;
    GCregistry = init_GenericSet(
        _compare_rawptr, 
        _hash_RtObject_ptr, 
        (void (*)(void *))_free_RtObject);
}



void cleanup_GarbageCollector() {
    liveObjCount = 0;
    gc_active=false;
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
void garbageCollect() {
    RtObject **all_active_obj = (RtObject **)GenericSet_to_list(GCregistry);
    // assert(GCregistry->size == (int)liveObjCount);
    
    for(unsigned long i=0; i < liveObjCount; i++) {
        all_active_obj[i]->mark=false;
    }

    CallFrame **callStack = getCallStack();

    // performs a depth first search on all live objects
    for(int i=0; i <= getCallStackPointer(); i++) {
        RtObject **elements = IdentifierTable_to_list(callStack[i]->lookup);
        for(int j=0; elements[j] != NULL; j++) {
            traverse_reference_graph(elements[j]);
        }
        free(elements);
    }

    // Frees all unreachable nodes
    for(unsigned long i=0 ; i < liveObjCount; i++) {
        if(!all_active_obj[i]->mark) {
            RtObject *tmp = remove_from_GC_registry(all_active_obj[i], true);
        }
    }

    free(all_active_obj);
}


/**
 * DESCRIPTION:
 * This object performs a DFS traversal of the reference graph, and marks all reachable nodes
*/
static void traverse_reference_graph(RtObject *root) {
    if(root->mark)
        return;
    
    root->mark = true;
    
    for(unsigned int i=0; i < root->outref_count; i++) {
        traverse_reference_graph(root->outrefs[i]);
    }

}

