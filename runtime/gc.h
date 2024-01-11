#include "rtobjects.h"
#include <stdbool.h>

bool is_GC_Active();
RtObject *add_to_GC_registry(RtObject *obj);
void init_GarbageCollector();
void cleanup_GarbageCollector();
void garbageCollect();