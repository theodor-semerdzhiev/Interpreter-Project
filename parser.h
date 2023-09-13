
/* Defines types for variables*/
enum variable_types
{
    NOT_SET,
    INT,
    DOUBLE,
    STRING,
    LIST,
    FUNC,
    VOID
};

/* Defines binary math operations */
enum bin_operation_types
{
    ADD,
    SUBSTRACT,
    MULT,
    DIV,
    AND,
    OR
};

/* Defines boolean operations */
enum bool_expression_types
{
    AND,
    OR,
    NOT
};

/* Type of parse tree nodes */
enum parse_tree_type
{
    IF,
    WHILE,
    FOR,
    ELSE,
    FUNCTION_CALL,
    FUNCTION_DECLARATION,
    VAR_DECLARATION,
    VAR_ASSIGNMENT,
    VAR_STANDALONE,
    BINARY_EXP,
    BINARY_OP,
    LITERAL,
};

/* Defines a constant value type */
enum literal_value_type
{
    INT,
    DOUBLE,
    STR
};

// Pre declares this struct
struct parse_tree_node;

/* Defines a if condition parse node */
struct if_node
{
    struct parse_tree_node *_branch_condition;
    struct parse_tree_node *_if_body;
    struct parse_tree_node *_else_block;
    struct parse_tree_node *_else_if_block;
};

/* Currently not being implemented */
struct for_node
{
};

/* Defines a while loop parse node */
struct while_node
{
    struct parse_tree_node *_branch_condition;
    struct parse_tree_node *_while_body;
};

/* Defines an else condition node */
struct else_node
{
    struct parse_tree_node *_branch_condition;
    struct parse_tree_node *_else_body;
};

/* Defines a function call node */
struct func_call_node
{
    struct parse_tree_node **args;
    char *ident;
};

/* Defines a function declaration node */
struct func_declaration_node
{
    enum variable_types return_type;
    struct parse_tree_node **args;
    struct parse_tree_node *func_body;
    char *ident;
};

/* Defines a variable declaration (creation) node */
struct var_declaration_node
{
    struct parse_tree_node *_assign_to;
    char *ident;
};

/* Defines a variable assignment node */
struct var_assignment_node
{
    struct parse_tree_node *_assign_to;
    char *ident;
};

/* Defines a standalone variable (or identifer) reference
 Used generally in other node types (i.e math expression, boolean expression )*/
struct var_reference_node
{
    enum variable_types type;
    char *ident;
};

/* Defines a boolean operation node */
struct bool_expression_node
{
    struct parse_tree_node *left;
    struct parse_tree_node *right;
    enum bool_expression_types type;
};

/* Defines a math operation node */
struct bin_operation_node
{
    struct parse_tree_node *left;
    struct parse_tree_node *right;
    enum bin_operation_types type;
};

/* Defines a literal (constant) declaration */
struct literal_value_node
{
    char *ident;
    enum literal_value_type type;
};

/*
    - This struct defines a general parser_tree_node
    - Depending on the type of node, the proper struct within the data union will be loaded from memory
*/
struct parse_tree_node
{

    /* Next node sharing the same 'scope' */
    struct parse_tree_node *next;

    /* Parent node */
    struct parse_tree_node *parent;

    /* The type of node */
    enum parse_tree_type type;

    /* Depending on the type the proper struct will be accessed */
    union data
    {
        struct if_node _if_node;
        struct while_node _while_node;
        struct for_node _for_node;
        struct else_node _else_node;
        struct func_call_node _func_call_node;
        struct func_declaration_node _func_declaration_node;
        struct var_declaration_node _var_declaration_node;
        struct var_assignment_node _var_assignment_node;
        struct var_reference_node _var_standalone_node;
        struct bool_expression_node _bin_exp_node;
        struct bin_operation_node _bin_op_node;
        struct literal_value_node _literal_val_node;
    };
};

struct parse_tree
{
    struct parse_tree_node *root;
};