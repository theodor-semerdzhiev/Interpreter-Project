void print_lexeme_arr_list(struct lexeme_array_list *lexemes);

void print_expression_component(struct expression_component *component, char* buffer, int rec_lvl);

void print_expression_tree(struct expression_node *root, char *buffer, int rec_lvl);
void print_ast_node(struct ast_node *node, char* buffer, int rec_lvl);
void print_ast_list(struct ast_list *list, char* buffer, int rec_lvl);
