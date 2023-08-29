#include <stdio.h>

/* Lexeme type */
enum lexeme_type {
  UNDEFINED='\0',
  WHITESPACE=' ',
  HASHTAG='#',
  DOT='.',
  SEMI_COLON=';',
  QUOTES='"',
  COMMA=',',
  OPEN_CURLY_BRACKETS='{',
  CLOSING_CURLY_BRACKETS='}',
  OPEN_PARENTHESIS='(',
  CLOSING_PARENTHESIS=')',
  OPEN_SQUARE_BRACKETS='[',
  CLOSING_SQUARE_BRACKETS=']',
  NEW_LINE='\n',
  KEYWORD='k',
  LITERALS='l',
  VARIABLE='v',
  AT_VARIABLE='@',
  IDENTIFIER='i'
};  

/* Structs for lexeme list*/
struct lexeme {
  enum lexeme_type type;
  char *ident;
  int line_num;
};

struct lexeme_array_list {
  struct lexeme **list;
  size_t len;
  size_t max_len;
};
/////////////////////////////////

/* Defines a single line */
struct line_construct {
  char* line;
  struct line_construct *next;
  int line_number;
};

/* Defined a list of lines */
struct line_list {
  struct line_construct *head;
  struct line_construct *tail;
  size_t length;
};

void parse_line_into_lexemes(struct lexeme_array_list *lexeme_arrlist, struct line_construct* line_struct);

void add_lexeme_to_arrlist(
  struct lexeme_array_list *arr, 
  enum lexeme_type type, 
  char* ident, 
  int line_num
);

void print_lexeme_arr_list(struct lexeme_array_list *lexemes);
struct lexeme_array_list* create_lexeme_arrlist(struct line_list* lines);
struct line_list* tokenize_string_by_newline(char *buffer);
char * c_read_file_condensed(const char * f_name);
void print_line_list(struct line_list *list);
void add_line_to_list(struct line_list *list, struct line_construct* line);
void free_line_list(struct line_list *list);
void free_lexeme_arrlist(struct lexeme_array_list* arr);
struct line_list* malloc_line_list();
struct line_construct* malloc_line_struct(char* line, int line_nb);
char *malloc_substring(char *g, int start, int end);