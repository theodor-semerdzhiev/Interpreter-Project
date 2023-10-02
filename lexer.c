#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "./lexer.h"
#include "./keywords.h"

#define DEFAULT_LEX_ARR_LENGTH 256

/*
All chars that are stand alone token unless otherwise specified
=, &, |, <, > are not included because they contain variants (i.e ==, >=, ||, &&, etc)
*/
static char *special_tokens = ".;,{}()[]:-+*/!%^";

/* Takes a 'special' char and returns its corresponding enumerator */
static enum lexeme_type get_special_char_type(const char c)
{
  switch (c)
  {
  case '.':
    return DOT;
  case ';':
    return SEMI_COLON;
  case ',':
    return COMMA;
  case '{':
    return OPEN_CURLY_BRACKETS;
  case '}':
    return CLOSING_CURLY_BRACKETS;
  case '(':
    return OPEN_PARENTHESIS;
  case ')':
    return CLOSING_PARENTHESIS;
  case '[':
    return OPEN_SQUARE_BRACKETS;
  case ']':
    return CLOSING_SQUARE_BRACKETS;
  case ':':
    return COLON;
  case '-':
    return MINUS_OP;
  case '+':
    return PLUS_OP;
  case '*':
    return MULT_OP;
  case '/':
    return DIV_OP;
  case '!':
    return LOGICAL_NOT_OP;
  case '%':
    return MOD_OP;
  case '^':
    return BITWISE_XOR_OP;
  default:
    return UNDEFINED;
  }
}

/* Checks if char c is a special character */
static bool is_char_special(const char c)
{
  for (int i = 0; i < strlen(special_tokens); i++)
  {
    if (c == special_tokens[i])
      return true;
  }
  return false;
}

/* Checks if string is a number, string must not have whitespace */
static bool is_token_numeric(char *token)
{
  int i = 0;
  if (token[0] == '-' || token[0] == '+')
    i++;

  for (; i < strlen(token); i++)
  {
    if (!isdigit(token[i]))
      return false;
  }
  return true;
}

/* Clears buffer and push its contents into the lexeme array list*/
static void clear_token_buffer_into_lexeme_arrlist(
    char *buffer,
    int buffer_ptr,
    struct lexeme_array_list *lexeme_arrlist,
    enum lexeme_type type,
    int line_number)
{

  if (buffer_ptr == 0)
    return;

  char *lexeme = (char *)malloc(sizeof(char) * buffer_ptr + 1);
  buffer[buffer_ptr] = '\0';
  strcpy(lexeme, buffer);

  // Checks the type of token
  if (type == UNDEFINED && buffer)
  {
    if (is_token_numeric(buffer))
      type = NUMERIC_LITERAL;
    else if (is_keyword(buffer))
      type = KEYWORD;
    else
      type = IDENTIFIER;
  }

  add_lexeme_to_arrlist(
      lexeme_arrlist, type, lexeme, line_number);
}


/* Parses line into lexemes and pushes it to the lexeme array list*/
void parse_line_into_lexemes(
    struct lexeme_array_list *lexeme_arrlist,
    struct line_construct *line_struct)
{
  char *line = line_struct->line;
  int i = 0;

  // skips first white space buffer
  while (isspace(line[i]) != 0 && line[i] != '\0')
    ++i;

  // checks if line is empty
  if (line[i] == '\0')
  {
    add_lexeme_to_arrlist(
        lexeme_arrlist, NEW_LINE, NULL, line_struct->line_number);
    return;
  }

  int line_len = strlen(line);
  char buffer[line_len];
  buffer[0] = '\0';
  int buffer_ptr = 0;

  while (i < line_len)
  {
    // comments are ignored
    if (line[i] == '#')
      break;

    // Handles case for literals
    if (line[i] == '"')
    {
      clear_token_buffer_into_lexeme_arrlist(
          buffer, buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number);
      buffer_ptr = 0;

      int literal_len = 0;
      int j = i + 1;
      while (j < line_len)
      {
        if (line[j] == '"' || line[j] == '\0')
          break;
        literal_len++;
        j++;
      }
      if (line[j] == '\0')
        break;

      char *literal = (char *)malloc(sizeof(char) * literal_len + 1);
      literal[literal_len] = '\0';
      for (int k = 0; k < literal_len; k++)
      {
        literal[k] = line[i + k + 1];
      }

      // add_lexeme_to_arrlist(
      //     lexeme_arrlist, QUOTES, NULL, line_struct->line_number);
      add_lexeme_to_arrlist(
          lexeme_arrlist, STRING_LITERALS, literal, line_struct->line_number);
      // add_lexeme_to_arrlist(
      //     lexeme_arrlist, QUOTES, NULL, line_struct->line_number);

      i += literal_len + 2;
      continue;
    }

    // Handles whitespace
    if (isspace(line[i]))
    {
      clear_token_buffer_into_lexeme_arrlist(
          buffer, buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number);
      buffer_ptr = 0;

      // add_lexeme_to_arrlist(
      //     lexeme_arrlist, WHITESPACE, NULL, line_struct->line_number);

      while (isspace(line[i]) && line[i] != '\0')
        ++i;

      // checks if line is empty
      if (line[i] == '\0')
        break;
      
      continue;
    }

    // handles special case chars and clears buffer
    if (line[i] == '<' || line[i] == '>' || line[i] == '=' || line[i] == '&' || line[i] == '|')
    {
      clear_token_buffer_into_lexeme_arrlist(
        buffer, buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number);
    }

    switch (line[i])
    {
      case '<':
      {
        switch (line[i + 1])
        {
        // handles shift left '<<' operator
        case '<':
        {
          add_lexeme_to_arrlist(lexeme_arrlist, SHIFT_LEFT_OP, NULL, line_struct->line_number);
          i += 2;
          break;
        }
        // handles lesser or equal '<=' operator
        case '=':
        {
          add_lexeme_to_arrlist(lexeme_arrlist, LESSER_EQUAL_OP, NULL, line_struct->line_number);
          i += 2;
          break;
        }
        // handles lesser '<' operator
        default:
        {
          add_lexeme_to_arrlist(lexeme_arrlist, LESSER_THAN_OP, NULL, line_struct->line_number);
          ++i;
        }
        }
        continue;
      }
      case '>':
      {
        switch (line[i+1])
        {
        // handles shift right '>>' operator
        case '>':
        {
          add_lexeme_to_arrlist(lexeme_arrlist, SHIFT_RIGHT_OP, NULL, line_struct->line_number);
          i += 2;
          break;
        }
        // handles greater or equal '>=' operator
        case '=':
        {
          add_lexeme_to_arrlist(lexeme_arrlist, GREATER_EQUAL_OP, NULL, line_struct->line_number);
          i += 2;
          break;
        }
        // handles greater '>' than logical operator
        default:
        {
          add_lexeme_to_arrlist(lexeme_arrlist, GREATER_THAN_OP, NULL, line_struct->line_number);
          ++i;
        }
        }
        continue;
      }
      case '=':
      {
        // handles logical equal '==' operator
        if (line[i + 1] == '=')
        {
          add_lexeme_to_arrlist(lexeme_arrlist, EQUAL_TO_OP, NULL, line_struct->line_number);
          i += 2;

          // handles assignment '=' operator
        }
        else
        {
          add_lexeme_to_arrlist(lexeme_arrlist, ASSIGNMENT_OP, NULL, line_struct->line_number);
          ++i;
        }
        continue;
      }
      case '&':
      {
        // logical and '&&' operator
        if (line[i + 1] == '&')
        {
          add_lexeme_to_arrlist(lexeme_arrlist, LOGICAL_AND_OP, NULL, line_struct->line_number);
          i += 2;

          // bitwise and '&' operator
        }
        else
        {
          add_lexeme_to_arrlist(lexeme_arrlist, BITWISE_AND_OP, NULL, line_struct->line_number);
          ++i;
        }
        continue;
      }
      // handles logical/bitwise or '||' operator
      case '|':
      {
        // logical and
        if (line[i + 1] == '|')
        {
          add_lexeme_to_arrlist(lexeme_arrlist, LOGICAL_OR_OP, NULL, line_struct->line_number);
          i += 2;

          // bitwise or '|' operator
        }
        else
        {
          add_lexeme_to_arrlist(lexeme_arrlist, BITWISE_AND_OP, NULL, line_struct->line_number);
          ++i;
        }
        continue;
      }
    }

    /* Handles attribute arrow '->' */
    if (line[i] == '-' && line[i + 1] == '>')
    {
      clear_token_buffer_into_lexeme_arrlist(
          buffer, buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number);
      buffer_ptr = 0;

      add_lexeme_to_arrlist(lexeme_arrlist, ATTRIBUTE_ARROW, NULL, line_struct->line_number);
      i += 2;
      continue;
    }

    // if its a special standalone char
    if (is_char_special(line[i]))
    {

      clear_token_buffer_into_lexeme_arrlist(
          buffer, buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number);
      buffer_ptr = 0;

      add_lexeme_to_arrlist(lexeme_arrlist, get_special_char_type(line[i]), NULL, line_struct->line_number);
    }
    else if (
        isalnum(line[i]) ||
        line[i] == '_')
    {

      buffer[buffer_ptr] = line[i];
      buffer_ptr++;
    }

    ++i;
  }

  // clears last of the buffer
  clear_token_buffer_into_lexeme_arrlist(
      buffer, buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number);
}

/* Creates and populates the array list */
struct lexeme_array_list *create_lexeme_arrlist(struct line_list *lines)
{
  struct lexeme_array_list *lexeme_arrlist = (struct lexeme_array_list *)malloc(sizeof(struct lexeme_array_list));

  // mallocs memory for array list
  struct lexeme **list = (struct lexeme **)malloc(sizeof(struct lexeme *) * DEFAULT_LEX_ARR_LENGTH + 1);
  lexeme_arrlist->len = 0;
  lexeme_arrlist->list = list;
  lexeme_arrlist->max_len = DEFAULT_LEX_ARR_LENGTH;

  // Sets the last index to NULL to know when the array terminates
  list[DEFAULT_LEX_ARR_LENGTH] = NULL;

  for (int i = 0; i < lines->length; i++)
  {
    parse_line_into_lexemes(lexeme_arrlist, lines->list[i]);
  }

  add_lexeme_to_arrlist(lexeme_arrlist, END_OF_FILE, NULL, lines->length);

  return lexeme_arrlist;
}

/* Mallocs lexeme struct */
struct lexeme *malloc_lexeme_struct(
  enum lexeme_type type,
  char *ident,
  int line_num)
{

  struct lexeme *lexeme = (struct lexeme *)malloc(sizeof(struct lexeme));
  lexeme->ident = ident;
  lexeme->line_num = line_num;
  lexeme->type = type;
  return lexeme;
}

/* frees lexeme array list */
void free_lexeme_arrlist(struct lexeme_array_list *arr)
{
  for (int i = 0; i < arr->len; i++)
  {
    free(arr->list[i]->ident);
    free(arr->list[i]);
  }
  free(arr->list);
  free(arr);
}

/* Adds lexeme to the lexeme array list */
void add_lexeme_to_arrlist(
    struct lexeme_array_list *arr,
    enum lexeme_type type,
    char *ident,
    int line_num)
{

  struct lexeme *lexeme = (struct lexeme *)malloc_lexeme_struct(
      type,
      ident,
      line_num);

  arr->list[arr->len] = lexeme;
  arr->len++;
  if (arr->len == arr->max_len)
  {
    struct lexeme **new_list = (struct lexeme **)malloc(sizeof(struct lexeme *) * arr->max_len * 2);

    for (int i = 0; i < arr->max_len; i++)
      new_list[i] = arr->list[i];

    free(arr->list);

    arr->list = new_list;
    arr->max_len *= 2;
  }
}

#define INITIAL_LINE_LIST_LENGTH 64
/* Parses string into linked list seperating text by new lines */
struct line_list *tokenize_string_by_newline(char *buffer)
{
  int line_number = 1;
  int buffer_ptr = 0;

  struct line_list *list = (struct line_list *)malloc(sizeof(struct line_list));
  list->max_length = INITIAL_LINE_LIST_LENGTH;
  list->list = malloc(sizeof(struct line_construct *) * INITIAL_LINE_LIST_LENGTH);
  list->length = 0;

  while (buffer[buffer_ptr] != '\0')
  {
    while (buffer[buffer_ptr] == '\n' && buffer[buffer_ptr] != '\0')
    {
      buffer_ptr++;
      line_number++;
    }

    if (buffer[buffer_ptr] == '\0')
      break;

    int line_len = 0;
    while (
        buffer[buffer_ptr + line_len] != '\n' &&
        buffer[buffer_ptr + line_len] != '\0')
    {

      line_len++;
    }

    char *line = malloc_substring(buffer, buffer_ptr, buffer_ptr + line_len);
    struct line_construct *line_struct = malloc_line_struct(line, line_number);

    add_line_to_line_list(list, line_struct);

    buffer_ptr += line_len;

    if (buffer[buffer_ptr] == '\0')
      break;
  }
  return list;
}

/* Prints line list to stdout */
void print_line_list(struct line_list *list)
{

  for (int i = 0; i < list->length; i++)
  {
    printf("Line %d: %s\n", list->list[i]->line_number, list->list[i]->line);
  }
}

/* Adds line to line list */
void add_line_to_line_list(struct line_list *list, struct line_construct *line)
{
  list->list[list->length] = line;
  list->length++;

  // if array list needs to be expanded
  if (list->length == list->max_length)
  {
    struct line_construct **new_list = malloc(sizeof(struct line_construct *) + list->length * 2);
    list->max_length *= 2;

    for (int i = 0; i < list->length; i++)
      new_list[i] = list->list[i];

    free(list->list);
    list->list = new_list;
  }
}

/* Frees line list */
void free_line_list(struct line_list *list)
{
  for (int i = 0; i < (int)list->length; i++)
  {
    free(list->list[i]->line);
    free(list->list[i]);
  }
  free(list->list);
  free(list);
}

/* Mallocs memory for linkedlist node (line) */
struct line_construct *malloc_line_struct(char *line, int line_nb)
{
  struct line_construct *line_struct = (struct line_construct *)malloc(sizeof(struct line_construct));
  line_struct->line = line;
  line_struct->line_number = line_nb;
  return line_struct;
}

/* Mallocs a substring from a base string */
char *malloc_substring(char *str, int start, int end)
{
  // the substring includes index start but does not include the end index.
  char *substr = malloc(sizeof(char) * (end - start) + 1);
  for (int i = 0; i < (end - start); i++)
    substr[i] = str[start + i];

  substr[end - start] = '\0';
  return substr;
}

/* Gets contents of file and stores it in heap */
char *get_file_contents(const char *f_name)
{
  FILE *file = fopen(f_name, "r");
  if (file == NULL)
    return NULL;

  char c = fgetc(file);
  int max_buffer_len = 512;
  int cur_buffer_len = 0;
  char *buffer = (char *)malloc(sizeof(char) * max_buffer_len + 1);

  while (c != EOF)
  {
    if (cur_buffer_len == max_buffer_len)
    {
      max_buffer_len *= 2;
      char *tmp_buffer = (char *)malloc(sizeof(char) * max_buffer_len + 1);
      for (int i = 0; i < cur_buffer_len; i++)
      {
        tmp_buffer[i] = buffer[i];
      }

      free(buffer);
      buffer = tmp_buffer;
    }
    buffer[cur_buffer_len] = c;
    c = fgetc(file);
    ++cur_buffer_len;
  }
  buffer[cur_buffer_len] = '\0';
  return buffer;
}
