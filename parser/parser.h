#pragma once

#include <stdbool.h>
#include <setjmp.h>
#include "lexer.h"
#include "../misc/memtracker.h"

typedef enum ParsingContext
{
    REGUALR_CTX,
    LIST_CTX,
    MAP_CTX,
    SET_CTX
} ParsingContext;

/* Top Level Object for Parser State */
typedef struct Parser
{
    int token_ptr;
    TokenList *token_list;

    MemoryTracker *memtracker; // keeps track of mallocs

    bool error_indicator;
    char *file_name;

    struct Lines
    {
        char **lines;
        int line_count;
    } lines;

    jmp_buf *error_handler; //

    ParsingContext ctx;
} Parser;

typedef enum expression_token_type
{
    PLUS,
    MINUS,
    MULT,
    DIV,
    MOD,
    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    SHIFT_LEFT,
    SHIFT_RIGHT,
    GREATER_THAN,
    GREATER_EQUAL,
    LESSER_THAN,
    LESSER_EQUAL,
    EQUAL_TO,
    LOGICAL_AND,
    LOGICAL_OR,
    VALUE
} ExpressionTokenType;

typedef enum expression_component_type
{
    NUMERIC_CONSTANT,
    STRING_CONSTANT,
    LIST_CONSTANT,
    NULL_CONSTANT,
    HASHMAP_CONSTANT,
    HASHSET_CONSTANT,
    //////////////

    VARIABLE,
    LIST_INDEX,
    FUNC_CALL,
    INLINE_FUNC,
} ExpressionComponentType;

/* pre defines some structs */
typedef struct expression_node ExpressionNode;
typedef struct AST_node AST_node;
typedef struct ast_list AST_List;
typedef struct expression_component ExpressionComponent;
typedef struct KeyValue KeyValue;
/*******************************************/

typedef struct expression_component
{
    enum expression_component_type type;

    ExpressionComponent *sub_component;
    ExpressionComponent *top_component;

    int line_num;

    int token_num; // Associated position in the Token List

    // all the data in this union is mutually exclusive
    union data
    {
        double numeric_const; // NUMERIC_CONSTANT type

        char *string_literal; // NUMERIC_CONSTANT type

        char *variable_reference; // for VARIABLE type (var reference)

        /* i.e the expression inside a list index, i.e syntax: [identifier][... exp] */
        ExpressionNode *list_index;

        /* for LIST_CONSTANT type, i.e syntax: [t_1, ..., t_n] */
        struct list_data
        {
            ExpressionNode **list_elements;
            int list_length;
        } list_const;

        /* for FUNC_CALL type, stores data about function call,
            i.e syntax: [identifer](arg_1, ... , arg_n) */
        struct func_data
        {
            /* i.e expression for each function call param */
            ExpressionNode **func_args;

            // number of arguments
            int args_num;

        } func_data;

        // defines HashMap built in data structure
        struct HashMap
        {
            KeyValue **pairs;
            int size;
        } HashMap;

        // defines HashSet built in data structure
        struct HashSet
        {
            ExpressionNode **values;
            int size;
        } HashSet;

        // used ONLY for inline defined functions (INLINE_FUNC type)
        AST_node *inline_func;

    } meta_data;
} ExpressionComponent;

typedef struct KeyValue
{
    ExpressionNode *key;
    ExpressionNode *value;
} KeyValue;

/* General struct for a expression */
typedef struct expression_node
{
    enum expression_token_type type;
    bool negation; // wether it as a ! op in front

    int token_num; // Associated position in the Token List

    ExpressionComponent *component; // contains a the 'value' of the node

    ExpressionNode *RHS;
    ExpressionNode *LHS;
} ExpressionNode;

/* Defines a abstract syntax tree type */
enum ast_node_type
{
    VAR_DECLARATION,
    VAR_ASSIGNMENT,
    IF_CONDITIONAL,
    ELSE_CONDITIONAL,
    ELSE_IF_CONDITIONAL,
    WHILE_LOOP,
    FUNCTION_DECLARATION,
    RETURN_VAL,
    LOOP_TERMINATOR,
    LOOP_CONTINUATION,

    EXPRESSION_COMPONENT,

    INLINE_FUNCTION_DECLARATION,

    OBJECT_DECLARATION
};

typedef enum access_modifer
{
    GLOBAL_ACCESS,  // global keyword -- visible between files (can only be used in a global scope )
    PRIVATE_ACCESS, // private keyword -- visible only within its object (field cannot be accessed via its object reference)
    PUBLIC_ACCESS,  // (no keyword) -- visible within its current scope, and via its object reference

    DOES_NOT_APPLY // for ast nodes that do not have access modifiers
} AccessModifier;

/* Represents the high level representation of the abstract syntax tree */
typedef struct AST_node
{
    // ast tree TYPE
    enum ast_node_type type;

    /*
    Only applies to:
        - VAR_DECLARATION
        - FUNCTION_DECLARATION
        - OBJECT_DECLARATION
    */
    AccessModifier access;

    int line_num;

    int token_num; // Associated position in the Token List

    // represents LHS identifer (for var assignment or declaration) or function name
    // union is NULL if type is a INLINE_FUNCTION_DECLARATION
    union identifier
    {
        // used for variable declaration (let keyword)
        char *declared_var;

        // use for function declaration
        char *func_name;

        // use for object declaration
        char *obj_name;

        // used for variable assignment, function calls or just standalone expression components
        ExpressionComponent *expression_component;
    } identifier;

    // contains extra information about the ast node
    union ast_data
    {
        // related expression (if, else if, loops, var assignment, var declaration, return)
        ExpressionNode *exp;

        // list of function prototype arguments (for both regular and inline functions )
        struct func_args
        {
            ExpressionNode **func_prototype_args;
            int args_num;
        } func_args;

        // list of object prototype arguments
        struct object_args
        {
            ExpressionNode **object_prototype_args;
            int args_num;
        } obj_args;

    } ast_data;

    /* Points to the child code block*/
    AST_List *body;

    /* Used to traverse ast_list */
    AST_node *next;
    AST_node *prev;

} AST_node;

/* Top level data structure for ast*/
typedef struct ast_list
{
    AST_node *head;
    AST_node *tail;

    size_t length;
    AST_node *parent_block;
} AST_List;

void init_Precedence();
Parser *init_Parser();
void free_parser(Parser *parser);
bool is_numeric_const_fractional(Parser *parser, int index);
bool is_lexeme_in_list(enum token_type type, const enum token_type list[], int list_length);

bool lexeme_lists_intersect(
    enum token_type list1[], const int list1_length, enum token_type list2[], const int list2_length);
double compute_fractional_double(Token *whole, Token *frac);
char *malloc_string_cpy(Parser *parser, const char *str);

ExpressionComponent *malloc_expression_component(Parser *parser);
ExpressionNode *malloc_expression_node(Parser *parser);
void free_expression_tree(ExpressionNode *root);
void free_expression_component(ExpressionComponent *component);

bool is_preliminary_expression_token(Token *lexeme);

ExpressionNode **parse_expressions_by_seperator(
    Parser *parser,
    enum token_type seperator,
    enum token_type end_of_exp);

KeyValue **parser_key_value_pair_exps(
    Parser *parser,
    enum token_type key_val_seperators,
    enum token_type pair_seperators,
    enum token_type end_of_exp);

int get_pointer_list_length(void **args);

ExpressionNode *parse_expression(
    Parser *parser,
    const enum token_type ends_of_exp[],
    const int ends_of_exp_length);

bool isOpToken(enum token_type type);



AST_node *malloc_ast_node(Parser *parser);
AST_List *malloc_ast_list(Parser *parser);
void free_ast_list(AST_List *list);
void free_ast_node(AST_node *node);

void push_to_ast_list(AST_List *list, AST_node *node);

/* Functions responsible for parsing code blocks */
AST_node *parse_variable_declaration(Parser *parser, int rec_lvl);
AST_node *parse_while_loop(Parser *parser, int rec_lvl);
AST_node *parse_if_conditional(Parser *parser, int rec_lvl);
AST_node *parse_else_conditional(Parser *parser, int rec_lvl);
AST_node *parse_loop_termination(Parser *parser, int rec_lvl);
AST_node *parse_loop_continuation(Parser *parser, int rec_lvl);
AST_node *parse_func_declaration(Parser *parser, int rec_lvl);
AST_node *parse_inline_func(Parser *parser, int rec_lvl);
AST_node *parse_object_declaration(Parser *parser, int rec_lvl);
AST_node *parse_variable_assignment_exp_func_component(Parser *parser, int rec_lvl);

AST_List *parse_code_block(
    Parser *parser,
    AST_node *parent_block,
    int rec_lvl,
    enum token_type ends_of_exp[],
    const int ends_of_exp_length);

