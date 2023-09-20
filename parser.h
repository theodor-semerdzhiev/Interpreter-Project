
enum bin_exp_token_type {
    PLUS,
    MINUS,
    MULT,
    DIV,
    MOD,
    CONSTANT,
    VAR
};

struct bin_exp_node {
    enum bin_exp_token_type type;

    union data {
        char* ident;
        int constant;
    } meta_data;

    struct bin_exp_node *RHS;
    struct bin_exp_node *LHS;
};

void reset_parser_state();
void set_parser_state(int _token_ptr, struct lexeme_array_list *_arrlist);

void free_parse_bin_exp(struct bin_exp_node *root);

double compute_exp(struct bin_exp_node *root);
struct bin_exp_node *parse_bin_exp(
    struct bin_exp_node *LHS,
    struct bin_exp_node *RHS,
    enum lexeme_type end_of_exp);