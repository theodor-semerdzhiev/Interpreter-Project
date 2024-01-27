#pragma once
#include "rtobjects.h"

bool rttype_isprimitive(RtType type);
const char *rtobj_type_toString(RtType type);
void rttype_freedata(RtType type, void *data, bool freerefs);