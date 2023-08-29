#include <stdbool.h>

struct keyword {
  char* keyword;
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

void init_keyword_table(); 
bool is_keyword(const char* token);