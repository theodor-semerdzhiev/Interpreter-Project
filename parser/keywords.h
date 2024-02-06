#pragma once 

#include <stdbool.h>

typedef enum keyword_type
{
  NOT_A_KEYWORD, // Define the case where its not a keyword

  LET_KEYWORD,
  FUNC_KEYWORD,
  RETURN_KEYWORD,
  BREAK_KEYWORD,
  IF_KEYWORD,
  ELSE_KEYWORD,
  WHILE_KEYWORD,
  FOR_KEYWORD,
  CONTINUE_KEYWORD,
  NULL_KEYWORD,
  GLOBAL_KEYWORD,
  PRIVATE_KEYWORD,
  OBJECT_KEYWORD,
  MAP_KEYWORD,
  SET_KEYWORD
  
} KeywordType;

void init_keyword_table();
const char* get_keyword_string(enum keyword_type keyword);
bool is_keyword(const char *token);
enum keyword_type get_keyword_type(const char *token);
void free_keyword_table();