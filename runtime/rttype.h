#pragma once
#include "rtobjects.h"

bool rrtype_has_data(RtType type);
bool rttype_isprimitive(RtType type);
const char *rtobj_type_toString(RtType type);
void rttype_freedata(RtType type, void *data, bool freerefs);
void rttype_set_GCFlag(void* data, RtType type, bool flag);
bool rttype_get_GCFlag(void *data, RtType type);
