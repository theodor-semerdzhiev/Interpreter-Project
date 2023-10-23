#include <stdbool.h>

enum keyword_type
{
  NOT_A_KEYWORD, // Define the case where its not a keyword

  LET_KEYWORD,
  FUNC_KEYWORD,
  RETURN_KEYWORD,
  BREAK_KEYWORD,
  IF_KEYWORD,
  ELSE_KEYWORD,
  WHILE_KEYWORD,
  CONTINUE_KEYWORD,
  NULL_KEYWORD,
  GLOBAL_KEYWORD,
  PRIVATE_KEYWORD,
  OBJECT_KEYWORD
};

struct keyword
{
  char *keyword;
  enum keyword_type type;
  struct keyword *next;
};

struct keyword_linked_list
{
  struct keyword *head;
  struct keyword *tail;
};

struct keywords_table
{
  struct keyword_linked_list **buckets;
  int nb_of_buckets;
};

extern char *keyword_list[];

void init_keyword_table();
const char* get_keyword_string(enum keyword_type keyword);
bool is_keyword(const char *token);
enum keyword_type get_keyword_type(const char *token);
void free_keyword_table();