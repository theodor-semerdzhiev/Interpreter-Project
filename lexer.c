#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "./lexer.h"
#include "./keywords.h"

#define DEFAULT_LEX_ARR_LENGTH 256

/* All chars that are stand alone token unless otherwise specified */
static char *special_tokens=".,;:{}()[]=-+/*&|!><";

/* Checks if char c is a special character */
static bool is_char_special(char c) {
  for(int i=0; i < strlen(special_tokens); i++) {
    if(c == special_tokens[i])
      return true;
  }
  return false;
}

/* Checks if string is a number, string must not have whitespace */
static bool is_token_numeric(char * token) {
  int i=0;
  if(token[0] == '-' || token[0] == '+') i++;

  for(;i < strlen(token); i++) {
    if(!isdigit(token[i])) 
      return false;
  }
  return true;
}

/* Clears buffer and push its contents into the lexeme array list*/
static void clear_token_buffer_into_lexeme_arrlist(
  char* buffer,
  int buffer_ptr,
  struct lexeme_array_list *lexeme_arrlist, 
  enum lexeme_type type, 
  int line_number) {

  if(buffer_ptr == 0) 
    return;
  
  char* lexeme = (char*)malloc(sizeof(char)*buffer_ptr+1);
  buffer[buffer_ptr]='\0';
  strcpy(lexeme, buffer);
  add_lexeme_to_arrlist(
    lexeme_arrlist, type, lexeme, line_number
  );
}

/* Prints out lexeme array list */
void print_lexeme_arr_list(struct lexeme_array_list *lexemes) {
  printf("Length: %zu\n", lexemes->len);

  for(int i=0; i < lexemes->len; i++) {
    char str[2];
    str[0]=lexemes->list[i]->type;
    str[1]='\0';

    char *type_in_str;

    switch(lexemes->list[i]->type) {
      case UNDEFINED:
        type_in_str="UNDEFINED";
        break;
      case WHITESPACE:
        type_in_str="WHITESPACE";
        break;
      case HASHTAG:
        type_in_str="#";
        break;
      case DOT:
        type_in_str="DOT";
        break;
      case SEMI_COLON:
        type_in_str="SEMI_COLON";
        break;
      case QUOTES:
        type_in_str="QUOTES";
        break;
      case COMMA:
        type_in_str="COMMA";
        break;
      case OPEN_CURLY_BRACKETS:
        type_in_str="{";
        break;
      case CLOSING_CURLY_BRACKETS:
        type_in_str="}";
        break;
      case OPEN_PARENTHESIS:
        type_in_str="(";
        break;
      case CLOSING_PARENTHESIS:
        type_in_str=")";
        break;
      case OPEN_SQUARE_BRACKETS:
        type_in_str="[";
        break;
      case CLOSING_SQUARE_BRACKETS:
        type_in_str="]";
        break;
      case EQUAL_SIGN:
        type_in_str="=";
        break;
      case MULT_OPERATOR:
        type_in_str="*";
        break;
      case DIV_OPERATOR:
        type_in_str="/";
        break;
      case PLUS_OPERATOR:
        type_in_str="+";
        break;
      case MINUS_OPERATOR:
        type_in_str="-";
        break;
      case COLON:
        type_in_str=":";
        break;
      case AND_OPERATOR:
        type_in_str="&";
        break;
      case OR_OPERATOR:
        type_in_str="|";
        break;
      case NOT_OPERATPR:
        type_in_str="!";
        break;
      case GREATER_THAN_OPERATOR:
        type_in_str=">";
        break;
      case LESSER_THAN_OPERATOR:
        type_in_str="<";
        break;
      case NEW_LINE:
        type_in_str="(ENTER)";
        break;
      case KEYWORD:
        type_in_str="KEYWORD";
        break;
      case LITERALS:
        type_in_str="LITERALS";
        break;
      case NUMERIC_LITERAL:
        type_in_str="NUMERIC_LITERAL";
        break;
      case VARIABLE:
        type_in_str="VARIABLE";
        break;
      case AT_VARIABLE:
        type_in_str="AT_VARIABLE";
        break;
      case IDENTIFIER:
         type_in_str="IDENTIFIER";
        break;
    }
    printf("[%d] Type: %s       ident:%s\n", 
    lexemes->list[i]->line_num, 
    type_in_str,
    lexemes->list[i]->ident);
  }
}

/* Parses line into lexemes and pushes it to the lexeme array list*/
void parse_line_into_lexemes(
struct lexeme_array_list *lexeme_arrlist, 
struct line_construct* line_struct) {
  char* line=line_struct->line;
  int i=0; 

  
  // skips first white space buffer
  while(isspace(line[i]) != 0 && line[i] != '\0')    
    ++i;  

  //checks if line is empty
  if(line[i] == '\0') {
    add_lexeme_to_arrlist(
      lexeme_arrlist, NEW_LINE, NULL,line_struct->line_number
    );
    return;
  }

  int line_len=strlen(line);
  char buffer[line_len];
  buffer[0]='\0';
  int buffer_ptr=0;

  while(i < line_len) {
    //comments are ignored
    if(line[i] == '#') 
      break;

    // Handles case for literals
    if(line[i] == '"') {
      clear_token_buffer_into_lexeme_arrlist(
        buffer,buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number
      );
      buffer_ptr=0;

      int literal_len=0;
      int j=i+1;
      while(j < line_len) {
        if(line[j] == '"' || line[j] == '\0') break;
        literal_len++;
        j++;
      }
      if(line[j] == '\0') break;

      char *literal = (char*)malloc(sizeof(char)*literal_len+1);
      literal[literal_len]='\0';
      for(int k=0; k < literal_len; k++) {
        literal[k]=line[i+k+1];
      }

      add_lexeme_to_arrlist(
        lexeme_arrlist, QUOTES, NULL, line_struct->line_number
      );
      add_lexeme_to_arrlist(
        lexeme_arrlist, LITERALS , literal, line_struct->line_number
      );
      add_lexeme_to_arrlist(
        lexeme_arrlist, QUOTES, NULL, line_struct->line_number
      );

      i+=literal_len+2;
      continue;
    }

    // Handles whitespace
    if(isspace(line[i])) {

      while(isspace(line[i]) && line[i] != '\0')    
        ++i;
      //checks if line is empty
      if(line[i] == '\0')
        break;

      clear_token_buffer_into_lexeme_arrlist(
        buffer,buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number);
      buffer_ptr=0;
      continue;

    } 

    // if its a special standalone char
    if(is_char_special(line[i])) {

      clear_token_buffer_into_lexeme_arrlist(
        buffer,buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number);
      buffer_ptr=0;

      add_lexeme_to_arrlist(lexeme_arrlist,(enum lexeme_type) line[i],NULL,line_struct->line_number);

    } else if(
      isalnum(line[i]) || 
      line[i] == '_') {

      buffer[buffer_ptr]=line[i];
      buffer_ptr++;
    }

    ++i;
  }

  //clears last of the buffer
  clear_token_buffer_into_lexeme_arrlist(
    buffer,buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number
  );

  add_lexeme_to_arrlist(
    lexeme_arrlist, NEW_LINE,NULL, line_struct->line_number
  );
}

/* Creates and populates the array list */
struct lexeme_array_list* create_lexeme_arrlist(struct line_list* lines) {
  struct lexeme_array_list *lexeme_arrlist = (struct lexeme_array_list*)malloc(sizeof(struct lexeme_array_list));

  struct lexeme **list = (struct lexeme **)malloc(sizeof(struct lexeme*)*DEFAULT_LEX_ARR_LENGTH);
  lexeme_arrlist->len=0;
  lexeme_arrlist->list=list;
  lexeme_arrlist->max_len=DEFAULT_LEX_ARR_LENGTH;

  //loops across each line
  struct line_construct *ptr = lines->head;
  while(ptr != NULL) {
    parse_line_into_lexemes(lexeme_arrlist,ptr); 
    ptr=ptr->next;
  }

  // Sets the type for the lexemes that are not yet set 
  for(int i=0; i < lexeme_arrlist->len; i++) {
    if(lexeme_arrlist->list[i]->type == UNDEFINED) {
      if(lexeme_arrlist->list[i]->ident == NULL) continue;

      if(is_token_numeric(lexeme_arrlist->list[i]->ident))
        lexeme_arrlist->list[i]->type=NUMERIC_LITERAL;
      else if(is_keyword(lexeme_arrlist->list[i]->ident)) 
        lexeme_arrlist->list[i]->type=KEYWORD;
      else 
        lexeme_arrlist->list[i]->type=IDENTIFIER;
    }
  }

  return lexeme_arrlist;
}

/* Mallocs lexeme struct */
struct lexeme* malloc_lexeme_struct(
  enum lexeme_type type, 
  char* ident, 
  int line_num) {

  struct lexeme* lexeme = (struct lexeme*)malloc(sizeof(struct lexeme));
  lexeme->ident=ident;
  lexeme->line_num=line_num;
  lexeme->type=type;

  return lexeme;
} 

/* frees lexeme array list */
void free_lexeme_arrlist(struct lexeme_array_list* arr) {
  for(int i=0; i < arr->len; i++) { 
    free(arr->list[i]->ident);
    free(arr->list[i]);
  }
  free(arr);
}

/* Adds lexeme to the lexeme array list */
void add_lexeme_to_arrlist(
  struct lexeme_array_list *arr, 
  enum lexeme_type type, 
  char* ident, 
  int line_num) {
  
  struct lexeme *lexeme = (struct lexeme*)malloc_lexeme_struct(
    type, 
    ident, 
    line_num
  );

  arr->list[arr->len]=lexeme;
  arr->len++;
  if(arr->len == arr->max_len) {
    struct lexeme **new_list = (struct lexeme **)malloc(sizeof(struct lexeme*)*arr->max_len*2);

    for(int i=0; i < arr->max_len; i++) 
      new_list[i]=arr->list[i];
    
    free(arr->list);

    arr->list=new_list;
    arr->max_len*=2;
  } 
}

/* Parses string into linked list seperating text by new lines */
struct line_list* tokenize_string_by_newline(char *buffer) {
  int line_number=1;
  int buffer_ptr=0;

  struct line_list *list = (struct line_list*)malloc(sizeof(struct line_list));
  list->head=NULL;
  list->tail=NULL;
  list->length=0;
  
  while(buffer[buffer_ptr] != '\0') {
    while(buffer[buffer_ptr] == '\n' && buffer[buffer_ptr] != '\0') {
      buffer_ptr++;
      line_number++;
    }
    
    if(buffer[buffer_ptr] == '\0') 
      break;
    
    int line_len = 0;
    while(
      buffer[buffer_ptr+line_len] != '\n' && 
      buffer[buffer_ptr+line_len] != '\0'
      ) {
      
      line_len++;
    }
    
    char *line = malloc_substring(buffer, buffer_ptr,buffer_ptr+line_len);

    struct line_construct *line_struct = malloc_line_struct(line, line_number);
    add_line_to_list(list, line_struct);
    
    buffer_ptr+=line_len;

    if(buffer[buffer_ptr] == '\0') 
      break;
  }
  return list;
}


/* Prints linked list to stdout */
void print_line_list(struct line_list *list) {
  struct line_construct *ptr = list->head;
  while(ptr != NULL) {
    printf("Line %d: %s\n", ptr->line_number,ptr->line);
    ptr=ptr->next;
  }
}

/* Adds line to linked list */
void add_line_to_list(struct line_list *list, struct line_construct* line) {
  if(list->head == NULL) {
    list->head=line;
    list->tail=line;
    list->length++;
    return;
  }

  list->tail->next=line;
  list->tail=line;
  list->length++;
}

/* Frees line list */
void free_line_list(struct line_list *list) {
  struct line_construct *ptr = list->head;
  while(ptr != NULL) {
    struct line_construct *tmp=ptr->next;
    free(ptr);
    ptr=tmp;
  }

  free(list);
}

/* Mallocs memory for linkedlist node (line) */
struct line_construct* malloc_line_struct(char* line, int line_nb) {
  struct line_construct *line_struct = (struct line_construct *)malloc(sizeof(struct line_construct));
  line_struct->line=line;
  line_struct->next=NULL;
  line_struct->line_number=line_nb;
  return line_struct;  
}

char *malloc_substring(char *g, int start, int end) {
  //the substring includes index start but does not include the end index.
  char *substr = malloc(sizeof(char)*(end - start)+1);
  int i;
  for(i = 0; i < (end - start); i++) {
      substr[i] = g[start + i];
  }
  substr[end - start]='\0';
  // printf("%s\n", substr);
  return substr;
}

/* Gets contents of file and removes comments */
char * c_read_file_condensed(const char * f_name) {
  FILE *file = fopen(f_name, "r");
  if(file == NULL) return NULL;
  
  char c = fgetc(file);
  int max_buffer_len=512;
  int cur_buffer_len=0;
  char * buffer = (char*)malloc(sizeof(char)*max_buffer_len+1);
  
  while(c != EOF) {
    if(cur_buffer_len == max_buffer_len) {
      max_buffer_len*=2;
      char *tmp_buffer = (char*)malloc(sizeof(char)*max_buffer_len+1);
      for(int i=0; i < cur_buffer_len; i++) {
        tmp_buffer[i]=buffer[i];
      }

      free(buffer);
      buffer = tmp_buffer;
    }
    buffer[cur_buffer_len]=c;
    c = fgetc(file);
    ++cur_buffer_len;
    
  }
  buffer[cur_buffer_len]='\0';
  return buffer;
}
