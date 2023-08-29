#include "./keywords.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define INITIAL_BUCKET_SIZE 50

static unsigned int hash(const char* keywords);
static void insert_keyword_to_table( char* keyword);

static struct keywords_table *keyword_table=NULL;

/* Stores all keywords */
static char *keyword_list[] = {
  "call",
  "move",
  "lw",
  "sw",
  "print"
  "int", 
  "str",
  "double",
  "\0"
};

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

  for(int i=0; *keyword_list[i] != '\0'; i++) {
    insert_keyword_to_table(keyword_list[i]);
  }

}

// Checks table to see if token is keyword
bool is_keyword(const char* token) {
  int index=hash(token) % keyword_table->nb_of_buckets;

  struct keyword_linked_list *list = keyword_table->buckets[index];
  struct keyword *ptr = list->head;
  
  while(ptr != NULL) {
    if(strcmp(token, ptr->keyword) == 0) return true;
    ptr=ptr->next;
  }

  return false;
}

// Inserts new keyword into table 
static void insert_keyword_to_table(char* keyword) {
  int index=hash(keyword) % keyword_table->nb_of_buckets;
  
  struct keyword_linked_list *list = keyword_table->buckets[index];
  struct keyword *keyword_node = (struct keyword*)malloc(sizeof(struct keyword));
  keyword_node->keyword=keyword;
  keyword_node->next=NULL;
  
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
  int hash=7; //prime number

  for(int i=0; i< strlen(keywords); i++) {
    hash = hash*31 + keywords[i];
  }
  return hash;
}