#include <stdio.h>
#include <stdlib.h>
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

  struct token_array_list *lexemes = create_lexeme_arrlist(list);

  print_lexeme_arr_list(lexemes);

  struct expression_node *tree = NULL;
  struct ast_list *ast = NULL;
  Parser *parser = NULL;

  if (lexemes->len > 1)
  {
    enum token_type end_of_program[] = {END_OF_FILE};

    parser = malloc_parser();
    parser->lexeme_list = lexemes;
    parser->lines = list;

    ast = parse_code_block(parser, NULL, 0, end_of_program, 1);
    print_ast_list(ast, "  ", 0);
  }

  if (tree)
    printf("%f\n", compute_exp(tree));

  free_ast_list(ast);
  free_expression_tree(tree);
  free_parser(parser);
  free_keyword_table();
  return 0;
}
