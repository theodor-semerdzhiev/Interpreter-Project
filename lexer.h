#include <stdlib.h>

/* Lexeme type */
enum token_type
{
    // Misc
    UNDEFINED,
    WHITESPACE,
    HASHTAG,

    ///////////////
    /* 'Special Tokens' */
    // Syntax tokens
    QUOTES,
    DOT,                     // .
    SEMI_COLON,              // ;
    COMMA,                   // ,
    OPEN_CURLY_BRACKETS,     // {
    CLOSING_CURLY_BRACKETS,  // }
    OPEN_PARENTHESIS,        // (
    CLOSING_PARENTHESIS,     // )
    OPEN_SQUARE_BRACKETS,    // [
    CLOSING_SQUARE_BRACKETS, // ]
    COLON,                   // :
    ATTRIBUTE_ARROW,         // ->

    // Math and Bitwise Operators

    ASSIGNMENT_OP, // =
    MULT_OP,       // *
    DIV_OP,        // /
    PLUS_OP,       // +
    MINUS_OP,      // -
    MOD_OP,        // %

    SHIFT_RIGHT_OP, // >>
    SHIFT_LEFT_OP,  // <<
    BITWISE_AND_OP, // &
    BITWISE_OR_OP,  // |
    BITWISE_XOR_OP, // ^

    // BOOLS
    LOGICAL_AND_OP,   // &&
    LOGICAL_OR_OP,    // ||
    LOGICAL_NOT_OP,   // !
    GREATER_THAN_OP,  // >
    LESSER_THAN_OP,   // <
    GREATER_EQUAL_OP, // >=
    LESSER_EQUAL_OP,  // <=
    EQUAL_TO_OP,      // ==

    /////////

    END_OF_FILE,     // end of file token
    KEYWORD,         // reserved keywords
    STRING_LITERALS, // "hello" ...
    NUMERIC_LITERAL, // numbers like 10230, 12, 23, etc
    IDENTIFIER       // reference to variables, functions etc
};

/* Structs for lexeme (a single token) list*/
typedef struct token
{
    enum token_type type;
    char *ident;
    int line_num;
    int line_pos;
} Token;

/* Array list for all lexeme (tokens) */
typedef struct token_array_list
{
    Token **list;
    size_t len;
    size_t max_len;
} TokenList;
/////////////////////////////////


// TODO
/* Top Level struct for lexer */
typedef struct lexer
{
    char *buffer;
    int buffer_size; // max size of buffer
    int buffer_ptr;  // current pointer to buffer

    int text_ptr; // pointer to string getting tokenized by lexer

    int cur_line; //

} Lexer;

/* Main lexing logic */
Lexer *malloc_lexer();
void free_lexer(Lexer *lexer);
TokenList *tokenize_str(Lexer *lexer, char *file_contents);
TokenList *cpy_token_list(TokenList *list);
TokenList *malloc_token_list();

/**********************/

/* Token array list */

void push_token(
    TokenList *arr,
    enum token_type type,
    char *ident,
    int line_num);

void free_token_list(TokenList *arr);
void print_token_list(TokenList *lexemes);
Token *malloc_token_struct(
    enum token_type type,
    char *ident,
    int line_num);
/*************************************/

/* Miscellaneous */
char *get_file_contents(const char *f_name);
char *malloc_substring(char *g, int start, int end);
/****************************************/
