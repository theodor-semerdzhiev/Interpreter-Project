/* Lexeme type */
enum lexeme_type
{
  // Misc
  UNDEFINED,
  WHITESPACE,
  HASHTAG,

  // Syntax tokens
  QUOTES,
  DOT, // .
  SEMI_COLON, // ;
  COMMA, // ,
  OPEN_CURLY_BRACKETS, // [
  CLOSING_CURLY_BRACKETS, // ]
  OPEN_PARENTHESIS, // (
  CLOSING_PARENTHESIS, // )
  OPEN_SQUARE_BRACKETS, // [
  CLOSING_SQUARE_BRACKETS, // ]
  COLON, // :
  ATTRIBUTE_ARROW, // ->

  // Math and Bitwise Operators 
  
  ASSIGNMENT_OP, // =
  MULT_OP, // *
  DIV_OP, // /
  PLUS_OP, // +
  MINUS_OP, // -
  MOD_OP, // %

  SHIFT_RIGHT_OP, // >>
  SHIFT_LEFT_OP, // <<
  BITWISE_AND_OP, // &
  BITWISE_OR_OP, // |
  BITWISE_XOR_OP, // ^

  // BOOLS
  LOGICAL_AND_OP, // &&
  LOGICAL_OR_OP, // ||
  LOGICAL_NOT_OP, // !
  GREATER_THAN_OP, // >
  LESSER_THAN_OP, // <
  GREATER_EQUAL_OP, // >=
  LESSER_EQUAL_OP, // <=
  EQUAL_TO_OP, // ==

  /////////

  NEW_LINE, // new line marker
  END_OF_FILE, // end of file token
  KEYWORD, // reserved keywords 
  STRING_LITERALS, // "hello" ...
  NUMERIC_LITERAL, // numbers like 10230, 12, 23, etc
  IDENTIFIER // reference to variables, functions etc
};

/* Structs for lexeme (a single token) list*/
struct lexeme
{
  enum lexeme_type type;
  char *ident;
  int line_num;
};

/* Array list for all lexeme (tokens) */
struct lexeme_array_list
{
  struct lexeme **list;
  size_t len;
  size_t max_len;
};
/////////////////////////////////

/* Defines a single line */
struct line_construct
{
  char *line;
  struct line_construct *next;
  int line_number;
};

/* Defined a list of lines */
struct line_list
{
  struct line_construct *head;
  struct line_construct *tail;
  size_t length;
};

/* Main lexing logic */

void parse_line_into_lexemes(struct lexeme_array_list *lexeme_arrlist, struct line_construct *line_struct);
/**********************/

/* Lexeme array list */

void add_lexeme_to_arrlist(
    struct lexeme_array_list *arr,
    enum lexeme_type type,
    char *ident,
    int line_num);
void free_lexeme_arrlist(struct lexeme_array_list *arr);
void print_lexeme_arr_list(struct lexeme_array_list *lexemes);
struct lexeme_array_list *create_lexeme_arrlist(struct line_list *lines);
/*************************************/

/* Line linked list */

void add_line_to_linked_list(struct line_list *list, struct line_construct *line);
void print_line_linked_list(struct line_list *list);
void free_line_linked_list(struct line_list *list);
struct line_construct *malloc_line_struct(char *line, int line_nb);
/***************************************/

/* Miscellaneous */

struct line_list *tokenize_string_by_newline(char *buffer);
char *get_file_contents(const char *f_name);
char *malloc_substring(char *g, int start, int end);
/****************************************/