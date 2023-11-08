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

//     ".", ";", ",", ":",                                      // seperators
//     "{", "}", "(", ")", "[", "]",                            // wrappers
//     "->",                                                    // attribute arrow
//     "-", "=", "*", "/", "+", "%", ">>", "<<", "&", "|", "^", // binary operators
//     "&&", "||", "!", ">", "<", ">=", "<=", "==",             // logical operators

/* Checks if string is a number, string must not have whitespace */
static bool is_token_numeric(char *token)
{
    int i = 0;
    if (token[0] == '-' || token[0] == '+')
        i++;

    for (; i < (int)strlen(token); i++)
    {
        if (!isdigit(token[i]))
            return false;
    }
    return true;
}

/* Returns the special token pointed to the lexer,
return UNDEFINED if its not a special token */
static enum token_type get_special_token_type(Lexer *lexer, char *string)
{

    char first = string[lexer->text_ptr];
    char second = string[lexer->text_ptr + 1];

    switch (first)
    {
    case '.':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return DOT;
    case ';':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return SEMI_COLON;
    case ',':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return COMMA;
    case ':':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return COLON;
    case '{':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return OPEN_CURLY_BRACKETS;
    case '}':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return CLOSING_CURLY_BRACKETS;
    case '(':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return OPEN_PARENTHESIS;
    case ')':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return CLOSING_PARENTHESIS;
    case '[':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return OPEN_SQUARE_BRACKETS;
    case ']':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return CLOSING_SQUARE_BRACKETS;
    case '*':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return MULT_OP;
    case '/':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return DIV_OP;
    case '+':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return PLUS_OP;
    case '%':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return MOD_OP;
    case '^':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return BITWISE_XOR_OP;
    case '!':
        lexer->text_ptr++;
        lexer->cur_pos++;
        return LOGICAL_NOT_OP;

    // - ->
    case '-':
    {
        lexer->text_ptr++;
        lexer->cur_pos++;
        switch (second)
        {
        case '>':
            lexer->text_ptr++;
            lexer->cur_pos++;
            return ATTRIBUTE_ARROW;
        default:
            return MINUS_OP;
        }
    }
    // = ==
    case '=':
    {
        lexer->text_ptr++;
        lexer->cur_pos++;
        switch (second)
        {
        case '=':
            lexer->text_ptr++;
            lexer->cur_pos++;
            return EQUAL_TO_OP;
        default:
            return ASSIGNMENT_OP;
        }
    }
    // > >> >=
    case '>':
    {
        lexer->text_ptr++;
        lexer->cur_pos++;
        switch (second)
        {
        case '>':
            lexer->text_ptr++;
            lexer->cur_pos++;
            return SHIFT_RIGHT_OP;
        case '=':
            lexer->text_ptr++;
            lexer->cur_pos++;
            return GREATER_EQUAL_OP;
        default:
            return GREATER_THAN_OP;
        }
    }
    // < << <=
    case '<':
    {
        lexer->text_ptr++;
        lexer->cur_pos++;
        switch (second)
        {
        case '<':
            lexer->text_ptr++;
            lexer->cur_pos++;
            return SHIFT_LEFT_OP;
        case '=':
            lexer->text_ptr++;
            lexer->cur_pos++;
            return LESSER_EQUAL_OP;
        default:
            return LESSER_THAN_OP;
        }
    }
    // & &&
    case '&':
    {
        lexer->text_ptr++;
        lexer->cur_pos++;
        switch (second)
        {
        case '&':
            lexer->text_ptr++;
            lexer->cur_pos++;
            return LOGICAL_AND_OP;
        default:
            return BITWISE_AND_OP;
        }
    }
    // | ||
    case '|':
    {
        lexer->text_ptr++;
        lexer->cur_pos++;
        switch (second)
        {
        case '|':
            lexer->text_ptr++;
            lexer->cur_pos++;
            return LOGICAL_OR_OP;
        default:
            return BITWISE_OR_OP;
        }
    }

    default:
        return UNDEFINED;
    }
}

/* Helper for pushing chars to lexer buffer */
static void push_char_to_buffer(Lexer *lexer, char c)
{
    lexer->buffer[lexer->buffer_ptr] = c;
    lexer->cur_pos++;
    
    if(lexer->buffer_ptr == 0) 
        lexer->prev_pos=lexer->cur_pos;
    
    lexer->buffer_ptr++;


    // resizes buffer
    if (lexer->buffer_ptr == lexer->buffer_size)
    {
        char *resized_buffer = malloc(sizeof(char) * (lexer->buffer_size * 2 + 1));
        for (int i = 0; i < lexer->buffer_ptr; i++)
        {
            resized_buffer[i] = lexer->buffer[i];
        }
        resized_buffer[lexer->buffer_ptr] = '\0';

        free(lexer->buffer);
        lexer->buffer = resized_buffer;
        lexer->buffer_size *= 2;
    }

    lexer->buffer[lexer->buffer_ptr] = '\0';

    if (c == '\n') {
        lexer->cur_line++;
        lexer->prev_pos=0;
        lexer->cur_pos=0;
    }
}

/* Resets buffer_ptr to 0 */
static void reset_buffer(Lexer *lexer)
{
    lexer->buffer_ptr = 0;
    lexer->buffer[0] = '\0';
}

/* Clear buffer by pushing token and resets buffer */
static void clear_buffer(Lexer *lexer, TokenList *list)
{
    if (lexer->buffer_ptr == 0)
        return;

    lexer->buffer[lexer->buffer_ptr] = '\0';
    char *str = malloc(sizeof(char) * (strlen(lexer->buffer) + 1));
    strcpy(str, lexer->buffer);
    push_token(list, UNDEFINED, str, lexer->cur_line, lexer->prev_pos);
    reset_buffer(lexer);
    lexer->prev_pos=lexer->cur_pos;
}

/* Creates new token list thats a copy */
TokenList *cpy_token_list(TokenList *list)
{
    TokenList *new_list = malloc_token_list();
    new_list->len = list->len;
    new_list->max_len = list->max_len;

    for (int i = 0; i < (int)new_list->len; i++)
    {
        new_list->list[i] = malloc_token_struct(
            list->list[i]->type,
            list->list[i]->ident,
            list->list[i]->line_num,
            list->list[i]->line_pos);
    }

    return new_list;
}

/* Parses file contents into a list of tokens */
TokenList *tokenize_str(Lexer *lexer, char *file_contents)
{
    TokenList *list = malloc_token_list();

    char *buffer = lexer->buffer;
    reset_buffer(lexer);

    while (file_contents[lexer->text_ptr] != '\0')
    {
        enum token_type type = get_special_token_type(lexer, file_contents);

        // if char is a special char
        if (type != UNDEFINED)
        {
            clear_buffer(lexer, list);
            lexer->prev_pos=lexer->cur_pos;
            push_token(list, type, NULL, lexer->cur_line, lexer->prev_pos);
            
            continue;
        }

        // string literal
        else if (file_contents[lexer->text_ptr] == '"')
        {
            clear_buffer(lexer, list);
            lexer->text_ptr++;

            while (
                file_contents[lexer->text_ptr] != '"' &&
                file_contents[lexer->text_ptr] != '\0')
            {
                push_char_to_buffer(lexer, file_contents[lexer->text_ptr]);
                lexer->text_ptr++;
            }

            char *literal = malloc(sizeof(char) * lexer->buffer_ptr + 1);
            strcpy(literal, buffer);

            push_token(list, STRING_LITERALS, literal, lexer->cur_line, lexer->prev_pos);
            reset_buffer(lexer);

            if (file_contents[lexer->text_ptr] == '\0')
                break;

            lexer->text_ptr++;

            continue;
        }

        // whitespace
        else if (isspace(file_contents[lexer->text_ptr]))
        {
            clear_buffer(lexer, list);

            while (isspace(file_contents[lexer->text_ptr]))
            {
                push_char_to_buffer(lexer, file_contents[lexer->text_ptr]);
                lexer->text_ptr++;
            }

            lexer->prev_pos=lexer->cur_pos;
            reset_buffer(lexer);

            continue;
        }
        // handles comments
        else if(file_contents[lexer->text_ptr] == '#') {
            while(file_contents[lexer->text_ptr] != '\n') {
                push_char_to_buffer(lexer,file_contents[lexer->text_ptr]);
                lexer->text_ptr++;
            }

            push_char_to_buffer(lexer,file_contents[lexer->text_ptr]);


            lexer->prev_pos=lexer->cur_pos;
            reset_buffer(lexer);
            lexer->text_ptr++;
            continue;
        }
        else
        {
            push_char_to_buffer(lexer, file_contents[lexer->text_ptr]);
        }

        lexer->text_ptr++;
    }

    clear_buffer(lexer, list);

    push_token(list, END_OF_FILE, NULL, lexer->cur_line, lexer->prev_pos);

    return list;
}

#define DEFAULT_LEXER_BUFFER_SIZE 100

/* Mallocs lexer */
Lexer *malloc_lexer()
{
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->buffer = malloc(sizeof(char) * (DEFAULT_LEXER_BUFFER_SIZE + 1));
    lexer->buffer_size = DEFAULT_LEXER_BUFFER_SIZE;
    lexer->buffer_ptr = 0;
    lexer->text_ptr = 0;
    lexer->cur_line = 1;
    lexer->cur_pos=0;
    lexer->prev_pos=0;
    return lexer;
}

/* Frees lexer */
void free_lexer(Lexer *lexer)
{
    free(lexer->buffer);
    free(lexer);
}

////////////////////////////////////////////////////

/* Mallocs token list */
TokenList *malloc_token_list()
{
    TokenList *token_list = (TokenList *)malloc(sizeof(TokenList));

    // mallocs memory for array list
    Token **list = (Token **)malloc(sizeof(Token *) * (DEFAULT_LEX_ARR_LENGTH + 1));
    token_list->len = 0;
    token_list->list = list;
    token_list->max_len = DEFAULT_LEX_ARR_LENGTH;

    return token_list;
}

/* Mallocs lexeme struct */
Token *malloc_token_struct(
    enum token_type type,
    char *ident,
    int line_num,
    int line_pos)
{

    Token *token = (Token *)malloc(sizeof(Token));
    token->ident = ident;
    token->line_num = line_num;
    token->type = type;
    token->line_pos=line_pos;
    return token;
}

/* frees lexeme array list */
void free_token_list(TokenList *arr)
{
    if (!arr)
        return;

    for (int i = 0; i < (int)arr->len; i++)
    {
        free(arr->list[i]->ident);
        free(arr->list[i]);
    }
    free(arr->list);
    free(arr);
}

/* Adds token to the lexeme array list */
void push_token(
    TokenList *arr,
    enum token_type type,
    char *ident,
    int line_num,
    int line_pos)
{
    // Checks the type of token
    if (type == UNDEFINED && ident)
    {
        if (is_token_numeric(ident))
            type = NUMERIC_LITERAL;
        else if (is_keyword(ident))
            type = KEYWORD;
        else
            type = IDENTIFIER;
    }

    Token *token = (Token *)malloc_token_struct(
        type,
        ident,
        line_num,
        line_pos);

    arr->list[arr->len] = token;
    arr->len++;

    if (arr->len == arr->max_len)
    {
        Token **new_list = (Token **)malloc(sizeof(Token *) * arr->max_len * 2);

        for (int i = 0; i < (int)arr->max_len; i++)
            new_list[i] = arr->list[i];

        free(arr->list);

        arr->list = new_list;
        arr->max_len *= 2;
    }
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

#define DEFAULT_BUFFER_LENGTH 512

/* Gets contents of file and stores it in heap */
char *get_file_contents(const char *f_name)
{
    FILE *file = fopen(f_name, "r");
    if (file == NULL)
        return NULL;

    char c = fgetc(file);
    int max_buffer_len = DEFAULT_BUFFER_LENGTH;
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
    fclose(file);
    return buffer;
}
