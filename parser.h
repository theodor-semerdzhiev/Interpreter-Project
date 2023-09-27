#include <stdbool.h>

enum expression_token_type
{
    PLUS,
    MINUS,
    MULT,
    DIV,
    MOD,
    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    SHIFT_LEFT,
    SHIFT_RIGHT,
    GREATER_THAN,
    LESSER_THAN,
    EQUAL_TO,
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,
    VALUE,
};

enum expression_component_type {
    NUMERIC_CONSTANT,
    STRING_CONSTANT,
    LIST_CONSTANT, //TODO

    VARIABLE,
    LIST_INDEX,
    FUNC_CALL
};

struct expression_node;

struct expression_component {
    enum expression_component_type type;

    struct expression_component *sub_component;

    // all the data in this union is mutually exclusive
    union data
    {
        double numeric_const; // NUMERIC_CONSTANT type

        char *string_literal; //NUMERIC_CONSTANT type

        char* ident; // for VARIABLE type

        /* i.e the expression inside [identifier][...] */
        struct expression_node *list_index;

        /* for FUNC_CALL type, stores data about function call,
            i,e [identifer](arg_1, ... , arg_n) */
        struct func_data { 
            /* i.e expression for each function call param */
            struct expression_node **func_args;
            int args_num; 
        } func_data;

    } meta_data;

    /* i.e the expression component after the '->' attribute arrow   */
    struct expression_component *attribute_arrow;
};

/* General struct for a expression */
struct expression_node
{
    enum expression_token_type type;

    struct expression_component *component; // contains a the 'value' of the node 

    struct expression_node *RHS;
    struct expression_node *LHS;
};

void reset_parser_state();
void set_parser_state(int _token_ptr, struct lexeme_array_list *_arrlist);
bool is_numeric_const_fractional(int index);
bool is_lexeme_in_list(enum lexeme_type type, enum lexeme_type list[], const int list_length);
double compute_fractional_double(struct lexeme *whole, struct lexeme *frac);
char *malloc_string_cpy(const char* str);

struct expression_component *malloc_expression_component();
struct expression_node *malloc_expression_node();

void free_expression_tree(struct expression_node *root);

double compute_exp(struct expression_node *root);

struct expression_component *parse_expression_component(
    struct expression_component *parent,
    int rec_lvl
);

struct expression_node **parse_function_args();
int get_argument_count(struct expression_node **args);

struct expression_node *parse_expression(
    struct expression_node *LHS,
    struct expression_node *RHS,
    enum lexeme_type ends_of_exp[],
    const int ends_of_exp_length);