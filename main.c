#include <stdio.h>
#include <stdlib.h>
#include "./lexer.h"
#include "./parser.h"
#include "./keywords.h"
#include "./dbgtools.h"

int main(int argc, char *argv[])
{
  init_keyword_table();

  char *file_contents = get_file_contents("test.txt");

  if (file_contents == NULL)
  {
    printf("Could not open %s\n", argv[1]);
    return 1;
  }

  struct line_list *list = tokenize_string_by_newline(file_contents);

  free(file_contents);

  struct lexeme_array_list *lexemes = create_lexeme_arrlist(list);
  print_lexeme_arr_list(lexemes);
  
  set_parser_state(0, lexemes);

  struct expression_node *tree = NULL;
  struct ast_list *ast = NULL;

  if (lexemes->len > 1)
  {
    // enum lexeme_type end_of_exp[] = {SEMI_COLON};
    // tree = parse_expression(NULL, NULL, end_of_exp, 1);
    // print_expression_tree(tree, "  ", 0);
    enum lexeme_type end_of_program[] = {END_OF_FILE};

    ast = parse_code_block(NULL, 0, end_of_program, 1);
    print_ast_list(ast, "  ",0);
  }
  reset_parser_state();

  if (tree)
    printf("%f\n", compute_exp(tree));
  
  free_ast_list(ast);

  free_expression_tree(tree);

  // print_line_list(list);
  free_lexeme_arrlist(lexemes);
  free_line_list(list);
  free_keyword_table();
  return 0;
}
