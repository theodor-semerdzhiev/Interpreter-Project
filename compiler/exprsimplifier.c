#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../parser/parser.h"
#include "../generics/utilities.h"

/**
 * DESCRIPTION:
 * This file contains logic for expression simplifier, which given a valid expression tree
 * It will simplify where possible
*/

static double apply_operation(enum expression_token_type op, double x, double y);

/**
 * DESCRIPTION:
 * Simplifies the input expression to prevent redundant computations
 * Returns, the input root
 * 
 * PARAMS:
 * root: expression node to be simplied
*/
ExpressionNode *simplify_expression(ExpressionNode *root) {
    // bases cases
    if(!root) return NULL;
    if(root->type == VALUE) {
        // if a negation can be performed
        if(root->negation && root->component && root->component->type == NUMERIC_CONSTANT) {
            root->component->meta_data.numeric_const = 
            !root->component->meta_data.numeric_const;
            root->negation=false;
        }
        return root;
    }

    root->LHS = simplify_expression(root->LHS);
    root->RHS = simplify_expression(root->RHS);

    if(!root->LHS || root->LHS->type != VALUE || !root->RHS || root->RHS->type != VALUE)
        return root;

    ExpressionComponent *lhs = root->LHS->component;
    ExpressionComponent *rhs = root->RHS->component;
    int token_num = lhs->token_num;

    // simplifying primitive operations with numbers
    if(lhs->type == NUMERIC_CONSTANT && rhs->type == NUMERIC_CONSTANT) {
        double num1 = lhs->meta_data.numeric_const;
        double num2 = rhs->meta_data.numeric_const;
        free_expression_tree(root->LHS); // free leaf
        free_expression_tree(root->RHS); // free leaf
        root->component= malloc_expression_component(NULL); 
        root->component->type=NUMERIC_CONSTANT;
        root->component->meta_data.numeric_const = apply_operation(root->type, num1, num2);
        root->component->token_num=token_num;
        root->type = VALUE;
        root->LHS=NULL;
        root->RHS=NULL;
        return root;
    }

    // adding strings
    if(root->type == PLUS && lhs->type == STRING_CONSTANT && rhs->type == STRING_CONSTANT) {
        char *str1 = lhs->meta_data.string_literal;
        char *str2 = rhs->meta_data.string_literal;
        root->component= malloc_expression_component(NULL); 
        root->component->type=STRING_CONSTANT;
        root->component->meta_data.string_literal = concat_strings(str1, str2);
        root->component->token_num=token_num;
        root->type = VALUE;
        free_expression_tree(root->LHS); // free leaf
        free_expression_tree(root->RHS); // free leaf
        root->LHS=NULL;
        root->RHS=NULL;
    }
    
    return root;
}

/**
 * DESCRIPTION:
 * Helper for applying operation on numbers
 * 
 * PARAMS:
 * op: operation to perform
 * x, y: inputs to compute
*/
static double apply_operation(enum expression_token_type op, double x, double y) {
    assert(op != VALUE);
    switch (op)
    {
        case PLUS: 
            return x + y;
        case MINUS: 
            return x - y;
        case MULT: 
            return x * y;
        case DIV: 
            return x / y;
        case MOD: 
            return x - ((x / y) * y);
        case BITWISE_AND: 
            return (int)x & (int)y;
        case BITWISE_OR: 
            return (int)x | (int)y;
        case BITWISE_XOR: 
            return (int)x ^ (int)y;
        case SHIFT_LEFT: 
            return (int)x << (int)y;
        case SHIFT_RIGHT: 
            return (int)x >> (int)y;
        case GREATER_THAN: 
            return x > y;
        case GREATER_EQUAL: 
            return x >= y;
        case LESSER_THAN: 
            return x < y;
        case LESSER_EQUAL: 
            return x <= y;
        case EQUAL_TO: 
            return x == y;
        case LOGICAL_AND: 
            return x && y;
        case LOGICAL_OR: 
            return x || y;
        default:
            break;
    }
    return 0;
}