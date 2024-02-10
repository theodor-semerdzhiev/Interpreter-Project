#pragma once
#include <stdlib.h>

typedef struct SetNode SetNode;

typedef struct RtSet {
    size_t size;
    size_t bucket_size;

    SetNode **buckets;

    bool GCFlag;
} RtSet;

#define DEFAULT_SET_BUCKETS 16