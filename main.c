#include <stdio.h>
#include <stdlib.h>
#include "./lexer.h"
#include "./parser.h"
#include "./keywords.h"

int main(int argc, char *argv[])
{
  init_keyword_table();

  char *file_contents = get_file_contents("test1.txt");

  if (file_contents == NULL)
  {
    printf("Could not open %s\n", argv[1]);
    return 1;
  }

  struct line_list *list = tokenize_string_by_newline(file_contents);

  free(file_contents);

  struct lexeme_array_list *lexemes = create_lexeme_arrlist(list);

  set_parser_state(0, lexemes);

  struct expression_node *tree = NULL;

  if (lexemes->len > 1) {
    enum lexeme_type end_of_exp[] = {SEMI_COLON};
    tree = parse_expression(NULL, NULL, end_of_exp, 1);

  }
  reset_parser_state();
  print_lexeme_arr_list(lexemes);

  if (tree)
    printf("%f\n", compute_exp(tree));

  free_expression_tree(tree);

  // print_line_list(list);
  free_lexeme_arrlist(lexemes);
  free_line_linked_list(list);
  free_keyword_table();
  return 0;
}
