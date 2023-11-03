#include <stdio.h>
#include <stdlib.h>
// #include "./parser.h"
#include "./keywords.h"
#include "./dbgtools.h"
#include "./semanalysis.h"

int main(int argc, char *argv[])
{
    init_keyword_table();

    char *file_contents = get_file_contents("test.txt");

    if (file_contents == NULL)
    {
      printf("Could not open %s\n", argv[1]);
      return 1;
    }

    // LineList *list = tokenize_string_by_newline(file_contents);

    // TokenList *tokens = create_token_list(list);

    Lexer *lexer = malloc_lexer();
    TokenList *tokens = _parse_line_into_tokens(lexer, file_contents);
    print_token_list(tokens);

    free(file_contents);

    enum token_type end_of_program[] = {END_OF_FILE};

    Parser *parser = malloc_parser();
    parser->lexeme_list = tokens;
    // parser->lines = list;
    parser->file_name = malloc_string_cpy(NULL, "test.txt");

    AST_List *ast = parse_code_block(parser, NULL, 0, end_of_program, 1);

    if (!parser->error_indicator)
    {
        print_ast_list(ast, "  ", 0);

        SemanticAnalyser *sem_analyser = malloc_semantic_analyser();
        bool is_sem_valid = AST_list_has_consistent_semantics(sem_analyser, ast);

        if (is_sem_valid)
        {
          printf("Valid semantics\n");
        }
        else
        {
          printf("Invalid semantics\n");
        }

        free_semantic_analyser(sem_analyser);
    }
    else
    {

        clear_memtracker_pointers(parser->memtracker);
        free_parser(parser);

        return 1;
    }


    free_ast_list(ast);
    free_parser(parser);
    // free_line_list(list);
    free_lexer(lexer);
    free_keyword_table();
    return 0;
}
