#include <stdbool.h>

enum bin_exp_token_type
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
    INTEGER_CONSTANT,
    DOUBLE_CONSTANT,
    VAR,
    LIST_INDEX,
    FUNC_CALL
};

struct expression_node
{
    enum bin_exp_token_type type;

    char *ident; // for VAR, LIST, and FUNC_CALL

    int args_num;

    union data
    {
        int integer_const;   // INTEGER_CONSTANT type
        double double_const; // DOUBLE_CONSTANT type

        /* i.e expression inside num[...]*/
        struct expression_node *list_index;

        /* i.e expression for each function call param */
        struct expression_node **func_args;
    } meta_data;

    struct expression_node *attribute_arrow;

    struct expression_node *RHS;
    struct expression_node *LHS;
};

void reset_parser_state();
void set_parser_state(int _token_ptr, struct lexeme_array_list *_arrlist);
bool is_numeric_const_double(int index);
bool is_lexeme_in_list(enum lexeme_type type, enum lexeme_type list[], const int list_length);

struct expression_node *malloc_expression_node();

void free_expression_tree(struct expression_node *root);

double compute_exp(struct expression_node *root);

struct expression_node **parse_function_args();
int get_argument_count(struct expression_node **args);

struct expression_node *parse_expression(
    struct expression_node *LHS,
    struct expression_node *RHS,
    enum lexeme_type ends_of_exp[],
    const int ends_of_exp_length);