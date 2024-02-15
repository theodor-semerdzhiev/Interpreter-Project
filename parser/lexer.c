#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "lexer.h"
#include "keywords.h"
#include "../generics/utilities.h"

#define DEFAULT_LEX_ARR_LENGTH 128

//     ".", ";", ",", ":",                                      // seperators
//     "{", "}", "(", ")", "[", "]",                            // wrappers
//     "->",                                                    // attribute arrow
//     "-", "=", "*", "/", "+", "%", ">>", "<<", "&", "|", "^", // binary operators
//     "&&", "||", "!", ">", "<", ">=", "<=", "==",             // logical operators
//     "#"                                                      // comment

/* Mallocs string for token */
static char *token_to_str(enum token_type type)
{
    switch (type)
    {
    case UNDEFINED:
        return cpy_string("UNDEFINED");
    case DOT:
        return cpy_string(".");
    case SEMI_COLON:
        return cpy_string(";");
    case QUOTES:
        return cpy_string("\"");
    case COMMA:
        return cpy_string(",");
    case OPEN_CURLY_BRACKETS:
        return cpy_string("{");
    case CLOSING_CURLY_BRACKETS:
        return cpy_string("}");
    case OPEN_PARENTHESIS:
        return cpy_string("(");
    case CLOSING_PARENTHESIS:
        return cpy_string(")");
    case OPEN_SQUARE_BRACKETS:
        return cpy_string("[");
    case CLOSING_SQUARE_BRACKETS:
        return cpy_string("]");
    case ASSIGNMENT_OP:
        return cpy_string("=");
    case MULT_OP:
        return cpy_string("*");
    case DIV_OP:
        return cpy_string("/");
    case PLUS_OP:
        return cpy_string("+");
    case MINUS_OP:
        return cpy_string("-");
    case MOD_OP:
        return cpy_string("%");
    case SHIFT_LEFT_OP:
        return cpy_string("<<");
    case SHIFT_RIGHT_OP:
        return cpy_string(">>");
    case BITWISE_AND_OP:
        return cpy_string("&");
    case BITWISE_OR_OP:
        return cpy_string("|");
    case BITWISE_XOR_OP:
        return cpy_string("^");
    case EXPONENT_OP:
        return cpy_string("**");
    case COLON:
        return cpy_string(":");
    case ATTRIBUTE_ARROW:
        return cpy_string("->");
    case LOGICAL_AND_OP:
        return cpy_string("&&");
    case LOGICAL_OR_OP:
        return cpy_string("||");
    case LOGICAL_NOT_OP:
        return cpy_string("!");
    case GREATER_THAN_OP:
        return cpy_string(">");
    case LESSER_THAN_OP:
        return cpy_string("<");
    case GREATER_EQUAL_OP:
        return cpy_string(">=");
    case LESSER_EQUAL_OP:
        return cpy_string("<=");
    case EQUAL_TO_OP:
        return cpy_string("==");
    case END_OF_FILE:
        return cpy_string("END_OF_FILE");
    case KEYWORD:
        return cpy_string("KEYWORD");
    case STRING_LITERALS:
        return cpy_string("STRING LITERALS");
    case NUMERIC_LITERAL:
        return cpy_string("NUMERIC_LITERAL");
    case IDENTIFIER:
        return cpy_string("IDENTIFIER");
    }
}

/* Returns the special token pointed to the lexer,
return UNDEFINED if its not a special token */
static enum token_type get_special_token_type(Lexer *lexer, char *string, size_t strlen)
{
    if (strlen <= (size_t)lexer->text_ptr)
        return UNDEFINED;
    char first = string[lexer->text_ptr];
    char second = strlen <= (size_t)lexer->text_ptr+1? '\0': string[lexer->text_ptr+1];

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
        switch (second)
        {
        case '*':
            lexer->text_ptr++;
            lexer->cur_pos++;
            return EXPONENT_OP;
        default:
            break;
        }
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

    // resets prev_pos and cur_pos if buffer is empty
    if (lexer->buffer_ptr == 0)
        lexer->prev_pos = lexer->cur_pos;

    lexer->buffer_ptr++;
    lexer->buffer[lexer->buffer_ptr] = '\0';

    // resizes buffer
    if (lexer->buffer_ptr == lexer->buffer_size)
    {
        char *resized_buffer = realloc(lexer->buffer, lexer->buffer_size * 2 + 1);
        if (resized_buffer)
        {
            lexer->buffer = resized_buffer;
            lexer->buffer_size *= 2;
        }
        else
        {
            fprintf(stderr, "Memory Error");
            return;
        }
    }

    lexer->buffer[lexer->buffer_ptr] = '\0';

    if (c == '\n')
    {
        lexer->cur_line++;
        lexer->prev_pos = 0;
        lexer->cur_pos = 0;
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
    lexer->prev_pos = lexer->cur_pos;
}

__attribute__((warn_unused_result))
/* Creates new token list thats a copy */
TokenList *
cpy_token_list(TokenList *list)
{
    TokenList *new_list = malloc_token_list();

    for (int i = 0; i < (int)list->len; i++)
    {
        push_token(
            new_list,
            list->list[i]->type,
            cpy_string(list->list[i]->ident),
            list->list[i]->line_num,
            list->list[i]->line_pos);
    }

    return new_list;
}

__attribute__((warn_unused_result))
/* Parses file contents into a list of tokens */
TokenList *
tokenize_str(Lexer *lexer, char *file_contents)
{
    TokenList *list = malloc_token_list();
    size_t len = strlen(file_contents);
    char *buffer = lexer->buffer;
    reset_buffer(lexer);

    while (len > (size_t)lexer->text_ptr && file_contents[lexer->text_ptr] != '\0')
    {
        enum token_type type = get_special_token_type(lexer, file_contents, len);

        // if char is a special char
        if (type != UNDEFINED)
        {
            clear_buffer(lexer, list);
            lexer->prev_pos = lexer->cur_pos;
            push_token(list, type, token_to_str(type), lexer->cur_line, lexer->prev_pos);

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
                // handles character negation
                if (file_contents[lexer->text_ptr] == '\\')
                {
                    if (file_contents[lexer->text_ptr + 1] == 'n')
                    {
                        push_char_to_buffer(lexer, '\n');
                        lexer->text_ptr++;
                    }
                    else
                    {
                        push_char_to_buffer(lexer, file_contents[++lexer->text_ptr]);
                    }
                    lexer->text_ptr++;
                }
                else
                {
                    push_char_to_buffer(lexer, file_contents[lexer->text_ptr++]);
                }
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

            lexer->prev_pos = lexer->cur_pos;
            reset_buffer(lexer);

            continue;
        }
        // handles comments
        else if (file_contents[lexer->text_ptr] == '#')
        {
            while (file_contents[lexer->text_ptr] != '\n' && file_contents[lexer->text_ptr] != '\0')
            {
                lexer->text_ptr++;
            }

            // pushes '/n' (to update fields within the lexer struct)
            push_char_to_buffer(lexer, file_contents[lexer->text_ptr]);

            lexer->prev_pos = lexer->cur_pos;
            reset_buffer(lexer);
        }
        else
        {
            push_char_to_buffer(lexer, file_contents[lexer->text_ptr]);
        }

        lexer->text_ptr++;
    }

    clear_buffer(lexer, list);

    push_token(list, END_OF_FILE, cpy_string("End of File"), lexer->cur_line, lexer->prev_pos);

    return list;
}

#define DEFAULT_LEXER_BUFFER_SIZE 100

__attribute__((warn_unused_result))
/* Mallocs lexer */
Lexer *
malloc_lexer()
{
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->buffer = malloc(sizeof(char) * (DEFAULT_LEXER_BUFFER_SIZE + 1));
    lexer->buffer_size = DEFAULT_LEXER_BUFFER_SIZE;
    lexer->buffer_ptr = 0;
    lexer->text_ptr = 0;
    lexer->cur_line = 1;
    lexer->cur_pos = 0;
    lexer->prev_pos = 0;
    return lexer;
}

/* Frees lexer */
void free_lexer(Lexer *lexer)
{
    free(lexer->buffer);
    free(lexer);
}

////////////////////////////////////////////////////

__attribute__((warn_unused_result))
/* Mallocs token list */
TokenList *
malloc_token_list()
{
    TokenList *token_list = (TokenList *)malloc(sizeof(TokenList));
    token_list->len = 0;
    token_list->list = (Token **)malloc(sizeof(Token *) * (DEFAULT_LEX_ARR_LENGTH + 1));
    token_list->max_len = DEFAULT_LEX_ARR_LENGTH;
    return token_list;
}

__attribute__((warn_unused_result))
/* Mallocs lexeme struct */
Token *
malloc_token_struct(
    enum token_type type,
    char *ident,
    int line_num,
    int line_pos)
{

    Token *token = (Token *)malloc(sizeof(Token));
    token->ident = ident;
    token->line_num = line_num;
    token->type = type;
    token->line_pos = line_pos;
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
        if (is_token_integer(ident))
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
        // Resize the array using realloc
        Token **new_list = realloc(arr->list, sizeof(Token *) * (arr->max_len * 2));

        if (new_list == NULL)
        {
            // Handle memory allocation failure
            return;
        }

        arr->list = new_list;
        arr->max_len *= 2;
    }
}

/* Tokenize string by a set of seperators */
char **tokenize_str_by_seperators(const char *input, const char sep, int *count)
{
    int i, j, len = strlen(input);
    int numSeparators = 0;

    for (i = 0; i < len; i++)
    {
        if (input[i] == sep)
        {
            numSeparators++;
        }
    }

    char **result = (char **)malloc((numSeparators + 2) * sizeof(char *)); // +2 to include the last substring
    *count = 0;

    int start = 0;

    // Iterate through the string to separate it
    for (i = 0; i <= len; i++)
    {
        if (input[i] == sep || input[i] == '\0')
        {
            result[*count] = (char *)malloc((i - start + 1) * sizeof(char));

            // Copy the substring to the result array
            for (j = start; j < i; j++)
            {
                result[*count][j - start] = input[j];
            }

            result[*count][j - start] = '\0';

            start = i + 1;
            (*count)++;
        }
    }

    return result;
}

__attribute__((warn_unused_result))
/* Creates a deep copy of 2D string array */
char **
cpy_2D_string_arr(char **strs, int strs_length)
{
    char **arr = malloc(sizeof(char *) * (strs_length + 1));
    int len, i;

    for (i = 0; i < strs_length; i++)
    {
        len = strlen(strs[i]);
        arr[i] = malloc(sizeof(char) * (len + 1));
        strcpy(arr[i], strs[i]);
        arr[i][len] = '\0';
    }

    arr[strs_length] = NULL;
    return arr;
}

#define DEFAULT_BUFFER_LENGTH 512

__attribute__((warn_unused_result))
/* Gets contents of file and stores it in heap */
char *
get_file_contents(const char *f_name)
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
        buffer[cur_buffer_len] = c;
        c = fgetc(file);
        ++cur_buffer_len;

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
    }
    buffer[cur_buffer_len] = '\0';
    fclose(file);
    return buffer;
}
