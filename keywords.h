#include <stdbool.h>

enum keyword_type {
  NOT_A_KEYWORD, // Define the case where its not a keyword

  LET,
  FUNC,
  RETURN,
  BREAK,
  IF,
  
  ELSE,
  WHILE,
  CONTINUE, 
  STR,
  DOUBLE,
  INT,
  LIST,
  VOID
};

struct keyword {
  char* keyword;
  enum keyword_type type;
  struct keyword *next;
};

struct keyword_linked_list {
  struct keyword *head;
  struct keyword *tail;
};

struct keywords_table {
  struct keyword_linked_list** buckets;
  int nb_of_buckets;
};

extern char *keyword_list[];

void init_keyword_table(); 
bool is_type_keyword(const char* token);
bool is_keyword(const char* token);
enum keyword_type get_keyword_type(const char* token);
void free_keyword_table();