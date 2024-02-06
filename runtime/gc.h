#pragma once
#include "rtobjects.h"
#include <stdbool.h>

bool is_GC_Active();
RtObject *add_to_GC_registry(RtObject *obj);
bool GC_Registry_has(const RtObject *obj);
RtObject *remove_from_GC_registry(RtObject *obj, bool free_rtobj);
void init_GarbageCollector();
void cleanup_GarbageCollector();
void garbageCollect();
void trigger_GC();