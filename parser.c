#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "keywords.h"
#include "lexer.h"
#include "parser.h"


/// @brief State of the parser (lexeme list and the lexeme pointer)
static int token_ptr=-1;
static struct lexeme_array_list *arrlist=NULL;

/* Resets the token_ptr and arrlist fields */
void reset_parser_state() {
    token_ptr=-1;
    arrlist=NULL;
}

/* Modifies the parser state */
void set_parser_state(int _token_ptr, struct lexeme_array_list *_arrlist) {
    token_ptr=_token_ptr;
    arrlist=_arrlist;
}


double compute_exp(struct bin_exp_node *root) {
    switch(root->type) {
        case VAR:
            return 10.0;
        case CONSTANT:
            return (double) root->meta_data.constant;
        case MULT: 
            return compute_exp(root->LHS)*compute_exp(root->RHS);
        case DIV:
            return compute_exp(root->LHS) / compute_exp(root->RHS);
        case PLUS:
            return compute_exp(root->LHS) + compute_exp(root->RHS);
        case MINUS:
            return compute_exp(root->LHS) - compute_exp(root->RHS);
        case MOD: {
            double lhs_d = compute_exp(root->LHS);
            double rhs_d = compute_exp(root->RHS);

            return lhs_d - floor(lhs_d / rhs_d)*rhs_d;
        }
    }
}

/* Frees binary expression parse tree */
void free_parse_bin_exp(struct bin_exp_node *root) {
    switch(root->type) {
        case CONSTANT:
            free(root);
            return;
        case VAR:
            free(root->meta_data.ident);
            free(root);
            return;
        default:
            free_parse_bin_exp(root->LHS);
            free_parse_bin_exp(root->RHS);
            free(root);
            return;
    }
}

// uses reverse polish notation
struct bin_exp_node *parse_bin_exp(
    struct bin_exp_node *LHS,
    struct bin_exp_node *RHS,
    enum lexeme_type end_of_exp) {

    assert(token_ptr != -1 && arrlist != NULL);
    struct lexeme **list = arrlist->list;

    if(list[token_ptr]->type == END_OF_FILE) return LHS;
    // Base case
    if(list[token_ptr]->type == end_of_exp) {
        token_ptr++;
        return LHS;
    }

    // Handles and Computes sub expressions
    if(list[token_ptr]->type == OPEN_PARENTHESIS) {
        token_ptr++;
        struct bin_exp_node *sub_exp = parse_bin_exp(NULL,NULL, CLOSING_PARENTHESIS);
        
        if(!LHS) return parse_bin_exp(sub_exp, RHS, end_of_exp);
        else return parse_bin_exp(LHS, sub_exp, end_of_exp);
        
    }

    struct bin_exp_node *node = malloc(sizeof(struct bin_exp_node));
    node->LHS=NULL;
    node->RHS=NULL;

    // handles variables
    if(list[token_ptr]->type == IDENTIFIER) {
        node->type=VAR;
        char* ident_cpy = malloc(sizeof(char)*strlen(list[token_ptr]->ident));
        strcpy(ident_cpy, list[token_ptr]->ident);
        
        node->meta_data.ident=ident_cpy;
        //
        token_ptr++;
        if(!LHS) return parse_bin_exp(node, RHS, end_of_exp);
        else if(!RHS) return parse_bin_exp(LHS, node, end_of_exp);
        else {
            free(node);
            return NULL; 
        }
    } 

    // handles constants
    if(list[token_ptr]->type == NUMERIC_LITERAL) {
        node->type= CONSTANT;
        node->meta_data.constant=atoi(list[token_ptr]->ident);
        token_ptr++;
        if(!LHS) return parse_bin_exp(node, RHS, end_of_exp);
        else if(!RHS) return parse_bin_exp(LHS, node, end_of_exp);
        else {
            free(node);
            return NULL; 
        }

    }

    // Handles math operators
    switch(list[token_ptr]->type) {
        case MULT_OP: node->type=MULT;
            break;
        case MINUS_OP: node->type=MINUS;
            break;
        case PLUS_OP: node->type=PLUS;
            break;
        case DIV_OP: node->type=DIV;
            break;
        case MOD_OP: node->type=MOD;
            break;
        default:
            free(node);
            
            return LHS;
    }

    node->LHS=LHS;
    node->RHS=RHS;

    token_ptr++;

    return parse_bin_exp(node, NULL, end_of_exp);
}