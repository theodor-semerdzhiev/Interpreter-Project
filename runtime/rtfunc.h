#include "rtobjects.h"

RtObject *mutate_func_data(RtObject *target, const RtObject *new_val, bool deepcpy);
void free_func_data(RtObject *obj, bool free_immutable);
