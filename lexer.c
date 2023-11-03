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
        return DOT;
    case ';':
        lexer->text_ptr++;
        return SEMI_COLON;
    case ',':
        lexer->text_ptr++;
        return COMMA;
    case ':':
        lexer->text_ptr++;
        return COLON;
    case '{':
        lexer->text_ptr++;
        return OPEN_CURLY_BRACKETS;
    case '}':
        lexer->text_ptr++;
        return CLOSING_CURLY_BRACKETS;
    case '(':
        lexer->text_ptr++;
        return OPEN_PARENTHESIS;
    case ')':
        lexer->text_ptr++;
        return CLOSING_PARENTHESIS;
    case '[':
        lexer->text_ptr++;
        return OPEN_SQUARE_BRACKETS;
    case ']':
        lexer->text_ptr++;
        return CLOSING_SQUARE_BRACKETS;
    case '*':
        lexer->text_ptr++;
        return MULT_OP;
    case '/':
        lexer->text_ptr++;
        return DIV_OP;
    case '+':
        lexer->text_ptr++;
        return PLUS_OP;
    case '%':
        lexer->text_ptr++;
        return MOD_OP;
    case '^':
        lexer->text_ptr++;
        return BITWISE_XOR_OP;
    case '!':
        lexer->text_ptr++;
        return LOGICAL_NOT_OP;

    // - ->
    case '-':
    {
        lexer->text_ptr++;
        switch (second)
        {
        case '>':
            lexer->text_ptr++;
            return ATTRIBUTE_ARROW;
        default:
            return MINUS_OP;
        }
    }
    // = ==
    case '=':
    {
        lexer->text_ptr++;
        switch (second)
        {
        case '=':
            lexer->text_ptr++;
            return EQUAL_TO_OP;
        default:
            return ASSIGNMENT_OP;
        }
    }
    // > >> >=
    case '>':
    {
        lexer->text_ptr++;
        switch (second)
        {
        case '>':
            lexer->text_ptr++;
            return SHIFT_RIGHT_OP;
        case '=':
            lexer->text_ptr++;
            return GREATER_EQUAL_OP;
        default:
            return GREATER_THAN_OP;
        }
    }
    // < << <=
    case '<':
    {
        lexer->text_ptr++;
        switch (second)
        {
        case '<':
            lexer->text_ptr++;
            return SHIFT_LEFT_OP;
        case '=':
            lexer->text_ptr++;
            return LESSER_EQUAL_OP;
        default:
            return LESSER_THAN_OP;
        }
    }
    // & &&
    case '&':
    {
        lexer->text_ptr++;
        switch (second)
        {
        case '&':
            lexer->text_ptr++;
            return LOGICAL_AND_OP;
        default:
            return BITWISE_AND_OP;
        }
    }
    // | ||
    case '|':
    {
        lexer->text_ptr++;
        switch (second)
        {
        case '|':
            lexer->text_ptr++;
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
static void _push_char_to_buffer(Lexer *lexer, char c)
{
    lexer->buffer[lexer->buffer_ptr] = c;
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

    if (c == '\n')
        lexer->cur_line++;
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
    push_token(list, UNDEFINED, str, lexer->cur_line);
    reset_buffer(lexer);
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
            list->list[i]->line_num);
    }
    
    return new_list;
}

/* Parses file contents into a list of tokens */
TokenList *_parse_line_into_tokens(Lexer *lexer, char *file_contents)
{
    TokenList *list = malloc_token_list();

    char *buffer = lexer->buffer;
    lexer->buffer_ptr = 0;

    while (file_contents[lexer->text_ptr] != '\0')
    {
        enum token_type type = get_special_token_type(lexer, file_contents);

        // if char is a special char
        if (type != UNDEFINED)
        {
            clear_buffer(lexer, list);
            push_token(list, type, NULL, lexer->cur_line);
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
                _push_char_to_buffer(lexer, file_contents[lexer->text_ptr]);
                lexer->text_ptr++;

            }

            char* literal = malloc(sizeof(char)*lexer->buffer_ptr+1);
            strcpy(literal,buffer);

            push_token(list, STRING_LITERALS, literal, lexer->cur_line);
            reset_buffer(lexer);

            if(file_contents[lexer->text_ptr] == '\0') 
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
                _push_char_to_buffer(lexer, file_contents[lexer->text_ptr]);
                lexer->text_ptr++;
            }

            reset_buffer(lexer);

            continue;
        }
        else
        {
            _push_char_to_buffer(lexer, file_contents[lexer->text_ptr]);
        }

        lexer->text_ptr++;
    }

    push_token(list, END_OF_FILE, NULL, lexer->cur_line);

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

////////////////////////////

/*
All chars that are stand alone token unless otherwise specified
=, &, |, <, > are not included because they contain variants (i.e ==, >=, ||, &&, etc)
*/
static const char *special_tokens = ".;,{}()[]:-+*/!%^";

/* Takes a 'special' char and returns its corresponding enumerator */
static enum token_type get_special_char_type(const char c)
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
    for (int i = 0; i < (int)strlen(special_tokens); i++)
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

    for (; i < (int)strlen(token); i++)
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
    TokenList *lexeme_arrlist,
    enum token_type type,
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

    push_token(
        lexeme_arrlist, type, lexeme, line_number);
}

/* Parses line into lexemes and pushes it to the lexeme array list*/
void parse_line_into_lexemes(
    TokenList *lexeme_arrlist,
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
            push_token(
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
            buffer_ptr = 0;
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
                push_token(lexeme_arrlist, SHIFT_LEFT_OP, NULL, line_struct->line_number);
                i += 2;
                break;
            }
            // handles lesser or equal '<=' operator
            case '=':
            {
                push_token(lexeme_arrlist, LESSER_EQUAL_OP, NULL, line_struct->line_number);
                i += 2;
                break;
            }
            // handles lesser '<' operator
            default:
            {
                push_token(lexeme_arrlist, LESSER_THAN_OP, NULL, line_struct->line_number);
                ++i;
            }
            }
            continue;
        }
        case '>':
        {
            switch (line[i + 1])
            {
            // handles shift right '>>' operator
            case '>':
            {
                push_token(lexeme_arrlist, SHIFT_RIGHT_OP, NULL, line_struct->line_number);
                i += 2;
                break;
            }
            // handles greater or equal '>=' operator
            case '=':
            {
                push_token(lexeme_arrlist, GREATER_EQUAL_OP, NULL, line_struct->line_number);
                i += 2;
                break;
            }
            // handles greater '>' than logical operator
            default:
            {
                push_token(lexeme_arrlist, GREATER_THAN_OP, NULL, line_struct->line_number);
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
                push_token(lexeme_arrlist, EQUAL_TO_OP, NULL, line_struct->line_number);
                i += 2;

                // handles assignment '=' operator
            }
            else
            {
                push_token(lexeme_arrlist, ASSIGNMENT_OP, NULL, line_struct->line_number);
                ++i;
            }
            continue;
        }
        case '&':
        {
            // logical and '&&' operator
            if (line[i + 1] == '&')
            {
                push_token(lexeme_arrlist, LOGICAL_AND_OP, NULL, line_struct->line_number);
                i += 2;

                // bitwise and '&' operator
            }
            else
            {
                push_token(lexeme_arrlist, BITWISE_AND_OP, NULL, line_struct->line_number);
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
                push_token(lexeme_arrlist, LOGICAL_OR_OP, NULL, line_struct->line_number);
                i += 2;

                // bitwise or '|' operator
            }
            else
            {
                push_token(lexeme_arrlist, BITWISE_AND_OP, NULL, line_struct->line_number);
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

            push_token(lexeme_arrlist, ATTRIBUTE_ARROW, NULL, line_struct->line_number);
            i += 2;
            continue;
        }

        // if its a special standalone char
        if (is_char_special(line[i]))
        {

            clear_token_buffer_into_lexeme_arrlist(
                buffer, buffer_ptr, lexeme_arrlist, UNDEFINED, line_struct->line_number);
            buffer_ptr = 0;

            push_token(lexeme_arrlist, get_special_char_type(line[i]), NULL, line_struct->line_number);
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
TokenList *create_token_list(LineList *lines)
{
    TokenList *lexeme_arrlist = (TokenList *)malloc(sizeof(TokenList));

    // mallocs memory for array list
    struct token **list_ = (struct token **)malloc(sizeof(struct token *) * (DEFAULT_LEX_ARR_LENGTH + 1));
    lexeme_arrlist->len = 0;
    lexeme_arrlist->list = list_;
    lexeme_arrlist->max_len = DEFAULT_LEX_ARR_LENGTH;

    // Sets the last index to NULL to know when the array terminates
    list_[DEFAULT_LEX_ARR_LENGTH] = NULL;

    for (int i = 0; i < (int)lines->length; i++)
    {
        parse_line_into_lexemes(lexeme_arrlist, lines->list[i]);
    }

    push_token(lexeme_arrlist, END_OF_FILE, NULL, lines->length);

    return lexeme_arrlist;
}

/* Mallocs lexeme struct */
Token *malloc_token_struct(
    enum token_type type,
    char *ident,
    int line_num)
{

    Token *lexeme = (Token *)malloc(sizeof(Token));
    lexeme->ident = ident;
    lexeme->line_num = line_num;
    lexeme->type = type;
    return lexeme;
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

/* Adds lexeme to the lexeme array list */
void push_token(
    TokenList *arr,
    enum token_type type,
    char *ident,
    int line_num)
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
        line_num);

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

#define INITIAL_LINE_LIST_LENGTH 64
/* Parses string into linked list seperating text by new lines */
LineList *tokenize_string_by_newline(char *buffer)
{
    int line_number = 1;
    int buffer_ptr = 0;

    LineList *list = (LineList *)malloc(sizeof(LineList));
    list->max_length = INITIAL_LINE_LIST_LENGTH;
    list->list = malloc(sizeof(struct line_construct *) * INITIAL_LINE_LIST_LENGTH);
    list->length = 0;
    list->max_line_len = 0;

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
void print_line_list(LineList *list)
{
    for (int i = 0; i < (int)list->length; i++)
    {
        printf("Line %d: %s\n", list->list[i]->line_number, list->list[i]->line);
    }
}

/* Adds line to line list */
void add_line_to_line_list(LineList *list, struct line_construct *line)
{
    list->list[list->length] = line;
    list->length++;

    // if array list needs to be expanded
    if ((int)list->length == list->max_length)
    {
        struct line_construct **new_list = malloc(sizeof(struct line_construct *) + list->length * 2);
        list->max_length *= 2;

        for (int i = 0; i < (int)list->length; i++)
            new_list[i] = list->list[i];

        free(list->list);
        list->list = new_list;
    }

    // updates max line length
    if (list->max_line_len < (int)strlen(line->line))
        list->max_line_len = (int)strlen(line->line);
}

/* Frees line list */
void free_line_list(LineList *list)
{
    if (!list)
        return;

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
    fclose(file);
    return buffer;
}
