#include "./keywords.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define INITIAL_BUCKET_SIZE 25

static unsigned int hash(const char* keywords);
static void insert_keyword_to_table(char* keyword, enum keyword_type type);

static struct keywords_table *keyword_table=NULL;

// Initializes keyword table
void init_keyword_table() {
  keyword_table= (struct keywords_table*)malloc(sizeof(struct keywords_table));
  keyword_table->nb_of_buckets=INITIAL_BUCKET_SIZE;
  keyword_table->buckets=(struct keyword_linked_list**)malloc(sizeof(struct keyword_linked_list*)*INITIAL_BUCKET_SIZE);

  for(int i=0; i < INITIAL_BUCKET_SIZE; i++) {
    keyword_table->buckets[i]=(struct keyword_linked_list*)malloc(sizeof(struct keyword_linked_list));
    keyword_table->buckets[i]->head=NULL;
    keyword_table->buckets[i]->tail=NULL;
  }

  /* Adds all keywords to table */
  insert_keyword_to_table("let", LET);
  insert_keyword_to_table("func", FUNC);
  insert_keyword_to_table("return", RETURN);
  insert_keyword_to_table("break", BREAK);
  insert_keyword_to_table("if", IF);
  insert_keyword_to_table("else", ELSE);
  insert_keyword_to_table("while", WHILE);
  insert_keyword_to_table("continue", CONTINUE);
  insert_keyword_to_table("str", STR);
  insert_keyword_to_table("number", NUMBER);
  insert_keyword_to_table("null", _NULL);
  insert_keyword_to_table("list", LIST);


  
}

/* Frees the keyword table */
void free_keyword_table() {
  if(keyword_table == NULL) 
    return;

  for(int i=0; i < keyword_table->nb_of_buckets; i++) {
    struct keyword *ptr = keyword_table->buckets[i]->head;
    while(ptr != NULL) {
      struct keyword *tmp=ptr->next;
      free(ptr);
      ptr=tmp;
    }
    free(keyword_table->buckets[i]);
  }
  free(keyword_table->buckets);
  free(keyword_table);
  keyword_table=NULL;
}

/* Checks wether token represents a type keyword */
bool is_type_keyword(const char* token) {
  if(token == NULL) return false;

  int index=hash(token) % keyword_table->nb_of_buckets;

  struct keyword_linked_list *list = keyword_table->buckets[index];
  struct keyword *ptr = list->head;
  
  while(ptr != NULL) {
    if(strcmp(token, ptr->keyword) == 0) {
      if (ptr->type == NUMBER || ptr->type == STR || ptr->type == LIST)  
        return true;
      
      return false;
    }
    ptr=ptr->next;
  }

  return false;
}

// Checks table to see if token is keyword
bool is_keyword(const char* token) {
  if(token == NULL) return false;

  unsigned int index=hash(token) % keyword_table->nb_of_buckets;

  struct keyword_linked_list *list = keyword_table->buckets[index];
  struct keyword *ptr = list->head;
  
  while(ptr != NULL) {
    if(strcmp(token, ptr->keyword) == 0) return true;
    ptr=ptr->next;
  }

  return false;
}

/* Gets the type of keyword */
enum keyword_type get_keyword_type(const char* token) {
  if(token == NULL) return NOT_A_KEYWORD;

  int index=hash(token) % keyword_table->nb_of_buckets;

  struct keyword_linked_list *list = keyword_table->buckets[index];
  struct keyword *ptr = list->head;
  
  while(ptr != NULL) {
    if(strcmp(token, ptr->keyword) == 0) return ptr->type;
    ptr=ptr->next;
  }

  return NOT_A_KEYWORD;
}

// Inserts new keyword into table 
static void insert_keyword_to_table(char* keyword, enum keyword_type type) {
  unsigned int index=hash(keyword) % keyword_table->nb_of_buckets;
  
  struct keyword_linked_list *list = keyword_table->buckets[index];
  struct keyword *keyword_node = (struct keyword*)malloc(sizeof(struct keyword));
  keyword_node->keyword=keyword;
  keyword_node->next=NULL;
  keyword_node->type = type;
  
  if(list->head == NULL) {
    list->head=keyword_node;
    list->tail=keyword_node;
  } else {
    list->tail->next=keyword_node;
    list->tail=list->tail->next;
  }

}

// hash function: string -> int
static unsigned int hash(const char* keywords) {
  unsigned int hash=7; //prime number

  for(int i=0; i < (int)strlen(keywords); i++) {
    hash = hash*31 + keywords[i];
  }
  return hash;
}