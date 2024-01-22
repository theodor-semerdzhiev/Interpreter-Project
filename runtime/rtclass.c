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
 * func: constructor function bytecode
 * classname: name
*/
RtClass *init_RtClass(RtFunction *func, char *classname) {
    RtClass *class = malloc(sizeof(RtClass));
    if(!class) return NULL;
    class->attrs_table=init_RtMap(0);
    if(!class->attrs_table) {
        free(class);
        return NULL;
    }
    class->body = func;
    class->classname = classname;
    return class;
}

