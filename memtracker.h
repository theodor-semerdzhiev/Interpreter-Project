
typedef struct mallocnode MallocNode;

/* Linked list */
typedef struct mallocnode
{
    void *ptr;
    void (*free)(void *);
    MallocNode *next;
} MallocNode;

typedef struct memtracker
{
    int size;
    MallocNode *head;
    MallocNode *tail;
} MemoryTracker;

MemoryTracker *init_memtracker();
MallocNode *push_to_memtracker(MemoryTracker *memtracker, void *ptr, void (*free_func)(void *));
void clear_memtracker_pointers(MemoryTracker *memtracker);
void clear_memtracker_without_pointers(MemoryTracker *memtracker);
void free_memtracker(MemoryTracker *memtracker);
void free_memtracker_without_pointers(MemoryTracker *memtracker);