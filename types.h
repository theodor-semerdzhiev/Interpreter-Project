
enum type
{
    NUMBER_TYPE,
    STRING_TYPE,
    LIST_TYPE,
    FUNC_TYPE
};

struct type_node
{
    enum type type;

    union type_data
    {
        struct type_node **args; // For function types
        struct type_node *list_type;
    } data;
};

struct type_node *malloc_type_struct();