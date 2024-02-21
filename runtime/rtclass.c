#include <assert.h>
#include <string.h>
#include "gc.h"
#include "../generics/utilities.h"
#include "rtclass.h"
/**
 * This file contains all relevant files about Class type
*/

/**
 * DESCRIPTION:
 * Initializes a RtClass, a default RtMap is created and the closure_count and closures are set to 0 and NULL respectively.
 * 
 * NOTE:
 * Function will return NULL if malloc fails
 * 
 * PARAMS:
 * classname: name
*/
RtClass *init_RtClass(char *classname) {
    // assert(classname);
    RtClass *class = malloc(sizeof(RtClass));
    if(!class) return NULL;
    class->body = NULL;
    class->attrs_table = init_RtMap(0);
    if(!class->attrs_table) {
        free(class);
        return NULL;
    }
    class->classname = classname;
    class->GCFlag = false;
    return class;
}

/**
 * DESCRIPTION:
 * Creates a copy of a class object. 
 * 
 * NOTE:
 * If deepcpy is performed, then must be VERY careful, since deep copies are created and those objects need to be put into the GC.
 * This function DOES NOT do this for you
 * 
 * PARAMS:
 * class: class to copy
 * deepcpy: wether a deep copy should be performed on key/value pairs. Otherwise, there are passed directly by reference
 * add_to_GC: wether refs should be added to GC
*/
RtClass *rtclass_cpy(const RtClass *class, bool deepcpy, bool add_to_GC) {
    assert(class);
    RtClass *cpy = init_RtClass(class->classname);
    if(!cpy) return NULL;
    cpy->body = class->body;

    RtObject **list = rtmap_getrefs(class->attrs_table, true, true);

    for(unsigned int i = 0; list[i] != NULL;) {
        RtObject *key = deepcpy? rtobj_deep_cpy(list[i], add_to_GC): list[i];
        RtObject *val = deepcpy? rtobj_deep_cpy(list[i+1], add_to_GC): list[i+1];
        rtmap_insert(cpy->attrs_table, key, val);
        
        if(add_to_GC) {
            add_to_GC_registry(key);
            add_to_GC_registry(val);
        }

        i += 2;
    }

    assert(cpy->attrs_table->size == class->attrs_table->size);

    free(list);
    return cpy;
}

/**
 * DESCRIPTION:
 * Converts class object to string
 * 
 * 
*/
char *rtclass_toString(const RtClass *cls) {
    assert(cls);
    assert(cls->classname);
    char buffer[100+strlen(cls->classname)];
    buffer[0] = '\0';
    snprintf(buffer, 100, "%s.class@%p", cls->classname, cls);
    char *strcpy = cpy_string(buffer);
    if (!strcpy)
        MallocError();
    return strcpy;
}

/**
 * DESCRIPTION:
 * class: class object to free
 * free_refs: wether associated objects should be freed
 * free_immutable: wether immutable data should be freed
*/
void rtclass_free(RtClass *class, bool free_refs, bool free_immutable) {
    if(!class) return;
    rtmap_free(class->attrs_table, free_refs, free_refs, free_immutable);
    if(free_immutable)
        free(class->classname);
    
    free(class);
}
