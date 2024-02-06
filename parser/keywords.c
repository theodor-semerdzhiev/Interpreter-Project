#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./keywords.h"

#define INITIAL_BUCKET_SIZE 25


/**
 * DESCRIPTION:
 * This file contains the Keyword table implementation used by the lexer and parser.
 * Provides a practical API for recognizing keywords.
 * Lookup Table uses chaining for collisions.
*/

/**
 * Forward Declaration
*/
typedef struct Keyword Keyword;

/**
 * DESCRIPTION:
 * Defines a struct for storing keywords
 * Also acts as a linked list node
*/
typedef struct Keyword
{
    char *keyword; // keyword
    KeywordType type; // Defines the tyoe of key word
    Keyword *next;  // next element in the linked list
} Keyword;

/**
 * Defines the linked list used for the Hash table
*/
typedef struct keyword_linked_list
{
    Keyword *head;
    Keyword *tail;

} KeywordLList;

/**
 * DESCRIPTION:
 * Defines the Top level struct for the Keyword hash table
*/
typedef struct KeywordTable
{
    KeywordLList **buckets; // the buckets used by KeywordTable
    int nb_of_buckets;
} KeywordTable;

/**
 * Forward Declarations
*/
static unsigned int hash(const char *keywords);
static void insert_keyword_to_table(char *keyword, KeywordType type);

/**
 * DESCRIPTION:
 * Global instance of keyword table
 * Default value is NULL
*/
static KeywordTable *keyword_table = NULL;

/**
 * DESCRIPTION:
 * Initializes keyword table and populates it
*/
void init_keyword_table()
{
    if(keyword_table) return;

    keyword_table = (KeywordTable *)malloc(sizeof(KeywordTable));
    keyword_table->nb_of_buckets = INITIAL_BUCKET_SIZE;
    keyword_table->buckets = (KeywordLList **)malloc(sizeof(KeywordLList *) * INITIAL_BUCKET_SIZE);

    for (int i = 0; i < INITIAL_BUCKET_SIZE; i++)
    {
        keyword_table->buckets[i] = (KeywordLList *)malloc(sizeof(KeywordLList));
        keyword_table->buckets[i]->head = NULL;
        keyword_table->buckets[i]->tail = NULL;
    }

    /* Adds all keywords to table */
    insert_keyword_to_table("let", LET_KEYWORD);
    insert_keyword_to_table("func", FUNC_KEYWORD);
    insert_keyword_to_table("return", RETURN_KEYWORD);
    insert_keyword_to_table("break", BREAK_KEYWORD);
    insert_keyword_to_table("if", IF_KEYWORD);
    insert_keyword_to_table("else", ELSE_KEYWORD);
    insert_keyword_to_table("while", WHILE_KEYWORD);
    insert_keyword_to_table("for", FOR_KEYWORD);
    insert_keyword_to_table("continue", CONTINUE_KEYWORD);
    insert_keyword_to_table("null", NULL_KEYWORD);
    insert_keyword_to_table("global", GLOBAL_KEYWORD);
    insert_keyword_to_table("private", PRIVATE_KEYWORD);
    insert_keyword_to_table("class", OBJECT_KEYWORD); 
    insert_keyword_to_table("set", SET_KEYWORD); 
    insert_keyword_to_table("map", MAP_KEYWORD); 
}


/**
 * DESCRIPTION:
 * Returns the string associated the keyword table
 * 
 * PARAMS:
 * keyword: keyword type
*/
const char* get_keyword_string(enum keyword_type keyword) {
    switch(keyword) {
        case LET_KEYWORD: 
        return "let";
        case FUNC_KEYWORD:
        return "func";
        case RETURN_KEYWORD:
        return "return";
        case BREAK_KEYWORD:
        return "break";
        case IF_KEYWORD:
        return "if";
        case ELSE_KEYWORD:
        return "else";
        case WHILE_KEYWORD:
        return "while";
        case CONTINUE_KEYWORD:
        return "continue";
        case NULL_KEYWORD:
        return "null";
        case GLOBAL_KEYWORD:
        return "global";
        case PRIVATE_KEYWORD:
        return "private";
        case OBJECT_KEYWORD:
        return "object";
        case MAP_KEYWORD:
        return "map";
        case SET_KEYWORD:
        return "set";
        default:
        return NULL;
    }
}

/**
 * DESCRIPTION:
 * Frees the global instance of the keyword table
 * After its freed, the glocal static instance is set to NULL
*/
void free_keyword_table()
{
    if (keyword_table == NULL)
        return;

    for (int i = 0; i < keyword_table->nb_of_buckets; i++)
    {
        Keyword *ptr = keyword_table->buckets[i]->head;
        while (ptr != NULL)
        {
        Keyword *tmp = ptr->next;
        free(ptr);
        ptr = tmp;
        }
        free(keyword_table->buckets[i]);
    }
    free(keyword_table->buckets);
    free(keyword_table);
    keyword_table = NULL;
}

    // Checks table to see if token is keyword
    bool is_keyword(const char *token)
    {
    if (token == NULL)
        return false;

    unsigned int index = hash(token) % keyword_table->nb_of_buckets;

    KeywordLList *list = keyword_table->buckets[index];
    Keyword *ptr = list->head;

    while (ptr != NULL)
    {
        if (strcmp(token, ptr->keyword) == 0)
        return true;
        ptr = ptr->next;
    }

    return false;
}

/**
 * DESCRIPTION:
 * Takes a string and looks it up in the static Keyworld table, and returns its type
 * If intput string s not contained within keyword table, NOT_A_KEYWORD is returned 
 * 
 * PARAMS:
 * token: string to lookup in the keyword table
*/
KeywordType get_keyword_type(const char *token)
{
    // lazy initialization
    init_keyword_table();

    if (token == NULL)
        return NOT_A_KEYWORD;

    int index = hash(token) % keyword_table->nb_of_buckets;

    KeywordLList *list = keyword_table->buckets[index];
    Keyword *ptr = list->head;

    while (ptr != NULL)
    {
        if (strcmp(token, ptr->keyword) == 0)
        return ptr->type;
        ptr = ptr->next;
    }

    return NOT_A_KEYWORD;
}

/**
 * DESCRIPTION:
 * Helper for inserting a keyword and its corresponding type inside the global instance of the keyword table
 * 
 * PARAMS:
 * keyword: keyword string 
 * type: the keywords corresponding type
*/
static void insert_keyword_to_table(char *keyword, KeywordType type)
{
    // lazy initialization
    init_keyword_table();
    
    unsigned int index = hash(keyword) % keyword_table->nb_of_buckets;

    KeywordLList *list = keyword_table->buckets[index];
    Keyword *keyword_node = (Keyword *)malloc(sizeof(Keyword));
    keyword_node->keyword = keyword;
    keyword_node->next = NULL;
    keyword_node->type = type;

    if (list->head == NULL)
    {
        list->head = keyword_node;
        list->tail = keyword_node;
    }
    else
    {
        list->tail->next = keyword_node;
        list->tail = list->tail->next;
    }
}

/**
 * DESCRIPTION:
 * hash function for strings
*/
static unsigned int hash(const char *keywords)
{
    unsigned int hash = 7; // prime number

    for (int i = 0; i < (int)strlen(keywords); i++)
    {
        hash = hash * 31 + keywords[i];
    }
    return hash;
}