
struct stacknode
{
    void *ptr;
    void (*free)(void *);
    struct stacknode *above;
    struct stacknode *below;
};

struct memstack
{
    int size;
    struct stacknode *top;
    struct stacknode *bottom;
};

struct memstack *init_memstack();
struct stacknode *malloc_pointer_struct();
void push_to_memstack(void *ptr, void (*free_func)(void *));
void clear_memstack_ptrs();
void clear_memstack_without_ptrs();
void free_memstack_ptr_top_bottom(void *ptr);
void free_memstack_ptr_bottom_top(void *ptr);
void free_memstack();
void free_memstack_without_ptr();