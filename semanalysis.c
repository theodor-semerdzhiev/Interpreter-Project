#include <stdbool.h>
#include <assert.h>
#include "parser.h"
#include "symtable.h"

/* Checks that the expression as consistent identifiers */
bool check_exp_ident_semantics(SymbolTable *symtable, struct expression_node *root)
{
    // Bases cases 
    if (!root)
        return true;

    if (root->type == VALUE)
    {
        assert(root->component->type == VARIABLE);
        if (!symtable_has_sym(symtable, root->component->meta_data.variable_reference)) 
            return false;
        

        return true;
    }

    return check_expression_identifers_semantics(symtable, root->LHS) ||
           check_expression_identifers_semantics(symtable, root->RHS);
}

/* Makes sure expression component is semantically correct */
bool expression_component_semantics(SymbolTable *symtable, struct expression_component *node) {
    struct expression_component *tmp = node;
    while(!tmp->sub_component) 
        tmp=tmp->sub_component;
    
    assert(tmp->meta_data.variable_reference);
    return symtable_has_sym(symtable,tmp->meta_data.variable_reference); 
}

/* Checks that var assignment makes semantic sense */
bool var_assignment_consistent_semantics(SymbolTable *symtable, AST_node *node) {
    assert(node->type == VAR_ASSIGNMENT);
    if(!expression_component_semantics(symtable, node->identifier.expression_component))
        return false;

    if(!check_exp_ident_semantics(symtable, node->ast_data.exp))
        return false;
    
    return true;
}

// TODO 
/* Checks that variable is semantically (and to some extent syntax) */
bool function_declaration_semantics(SymbolTable *symtable, AST_node *node) {
    assert(node->type == FUNCTION_DECLARATION);

    for(int i=0; i < node->ast_data.func_args.args_num; i++) {

        if(node->ast_data.func_args.func_prototype_args[i]->type == INLINE_FUNCTION_DECLARATION) {
            // TODO 
            // recursively checks that inline function as consistent syntax
        } else if(node->ast_data.func_args.func_prototype_args[i]->type != VALUE) 
            // SYNTAX ERROR
            return false;
    }
    
    return true;
}

/* Checks that AST list has consistent semantics
- All variable reference are valid 
- Function declaration are valid
- Logical makes sense 
*/
bool AST_list_has_consistent_semantics(struct ast_list *ast_list)
{
    if (!ast_list)
        return true;

    AST_node *node = ast_list->head;

    SymbolTable *symtable = malloc_symbol_table();

    while (node)
    {
        bool tmp = true;
        switch (node->type)
        {
        case VAR_DECLARATION:
        {
            add_sym_to_symtable(symtable, node->identifier.declared_var);
            break;
        }
        case VAR_ASSIGNMENT:
        {
            tmp = var_assignment_consistent_semantics(symtable, node);
            break;
        }
        case IF_CONDITIONAL:
        {
            
        }
        
        case ELSE_IF_CONDITIONAL:
        {
            if(node->prev != IF_CONDITIONAL) {
                // TODO -> BAD SYNTAX
            } else {

            }
        }
        case WHILE_LOOP:
        {

        }
        case FUNCTION_DECLARATION:
        {
            tmp = function_declaration_semantics(symtable, node);
            break;
        }
        case RETURN_VAL:
        {
            
        }
        case LOOP_TERMINATOR:
        {
        }
        case LOOP_CONTINUATION:
        {
        }

        case EXPRESSION_COMPONENT:
        {

        }

        case INLINE_FUNCTION_DECLARATION:
        {
        }
        }
        if(!tmp) 
            return tmp;

        node = node->next;
    }

}