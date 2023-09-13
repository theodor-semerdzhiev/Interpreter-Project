#include <stdio.h>
#include <stdlib.h>
#include "./lexer.h"
#include "./keywords.h"

int main(int argc, char *argv[])
{
  init_keyword_table();

  char *file_contents = get_file_contents("test.txt");
  
  if (file_contents == NULL) {
    printf("Could not open %s\n", argv[1]);
    return 1;
  }

  struct line_list *list = tokenize_string_by_newline(file_contents);

  free(file_contents);

  struct lexeme_array_list *lexemes = create_lexeme_arrlist(list);

  // print_line_list(list);
  print_lexeme_arr_list(lexemes);
  free_line_linked_list(list);
  free_lexeme_arrlist(lexemes);
  free_keyword_table();
  return 0;
}
