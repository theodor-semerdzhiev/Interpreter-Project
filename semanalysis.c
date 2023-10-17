#include <stdbool.h>
#include <assert.h>
#include "parser.h"
#include "semanalysis.h"
#include "symtable.h"

/* Checks if the expression component type is terminal component */
static bool is_exp_component_terminal(enum expression_component_type type) {
    return 
    type == LIST_CONSTANT || 
    type == STRING_CONSTANT || 
    type == NUMERIC_CONSTANT;
}

/* Mallocs semantic analyser struct */
SemanticAnalyser *malloc_semantic_analyser()
{
    SemanticAnalyser *sem_analyser = malloc(sizeof(SemanticAnalyser));
    sem_analyser->is_in_loop = false;
    sem_analyser->nesting_lvl = 0;
    sem_analyser->scope_type = GLOBAL_SCOPE;
    sem_analyser->symtable = malloc_symbol_table();
    return sem_analyser;
}

/* Frees semantic analyser */
void free_semantic_analyser(SemanticAnalyser *sem_analyser)
{
    free_sym_table(sem_analyser->symtable);
    free(sem_analyser);
}

/* Checks that the expression as consistent identifiers */
bool exp_has_correct_semantics(SemanticAnalyser *sem_analyser, ExpressionNode *root)
{
    // Bases cases
    if (!root)
        return true;

    if (root->type == VALUE)
    {
        if (!expression_component_has_correct_semantics(sem_analyser, root->component))
            return false;

        return true;
    }

    // recursive calls
    return check_expression_identifers_semantics(sem_analyser, root->LHS) &&
           check_expression_identifers_semantics(sem_analyser, root->RHS);
}

// TODO
/* Makes sure expression component is semantically correct */
bool expression_component_has_correct_semantics(SemanticAnalyser *sem_analyser, ExpressionComponent *node)
{
    if(node->sub_component) {
        if(!expression_component_has_correct_semantics(sem_analyser, node)) {
            return false;
        }
    }
    
    switch(node->type) {
        case NUMERIC_CONSTANT: {
            if(node->top_component || node->sub_component) {
                // Invalid Syntax: 200->[EXPRESSION COMPONENT], not valid
                return false;
            }

            return true;
        }
        case STRING_CONSTANT: {
            if(node->top_component || node->sub_component) {
                // Invalid Syntax: "123123"->[EXPRESSION COMPONENT] not valid
                return false;
            }

            return true;
        }

        case LIST_CONSTANT: {
            if(node->top_component &&
                node->top_component->type != IDENTIFIER) {
                // Invalid Syntax: [1,2,3,4,5,6,7,8]
                return false;
            }
            
            return true;
        }

        case VARIABLE: {
            if(node->top_component && 
                is_exp_component_terminal(node->top_component->type)) {
                // Invalid Syntax: num -> [TERMINAL EXPRESSION COMPONENT]

                return false;
            }
        }

        case LIST_INDEX: {
            if(node->sub_component->type != IDENTIFIER || 
                node->sub_component->type != FUNC_CALL) {
                // Invalid syntax: [1,2,3,4,5]
                return false;
            }

            if(!exp_has_correct_semantics(sem_analyser, node->meta_data.list_index)) 
                return true;
            
            

            
        }
        //TODO
        case FUNC_CALL: {
            break;
        }

        //TODO
        case INLINE_FUNC: {
            break;
        }
    }

    return false;
}

/* Checks that var assignment makes semantic sense */
bool var_assignment_has_correct_semantics(SemanticAnalyser *sem_analyser, AST_node *node)
{
    assert(node->type == VAR_ASSIGNMENT);
    return expression_component_has_correct_semantics(sem_analyser, node->identifier.expression_component) &&
           exp_has_correct_semantics(sem_analyser, node->ast_data.exp);
}

/* Checks function declaration semantics (and to some extent syntax) */
bool function_declaration_semantics(SemanticAnalyser *sem_analyser, AST_node *node)
{
    assert(node->type == FUNCTION_DECLARATION);

    for (int i = 0; i < node->ast_data.func_args.args_num; i++)
    {
        if (node->ast_data.func_args.func_prototype_args[i]->type != VALUE)
            return false;

        // makes sure all func args are identifiers
        if (expression_component_has_correct_semantics(
            sem_analyser,
            node->ast_data.func_args.func_prototype_args[i]->component))

            return false;
    }

    return true;
}

/* Checks that AST list has consistent semantics
- All variable references are valid
- Function declarations are valid
- Program makes logical sense
*/
bool AST_list_has_consistent_semantics(SemanticAnalyser *sem_analyser, AST_List *ast_list)
{
    if (!ast_list)
        return true;

    AST_node *node = ast_list->head;

    while (node)
    {
        switch (node->type)
        {
        
        // i.e let var = ...
        case VAR_DECLARATION:
        {
            // saves symbol
            add_sym_to_symtable(
                sem_analyser->symtable,
                node->identifier.declared_var,
                sem_analyser->nesting_lvl);
            break;
        }

        // i.e var = ...
        case VAR_ASSIGNMENT:
        {
            if (!var_assignment_has_correct_semantics(sem_analyser, node))
                return false;

            break;
        }

        //i.e if(EXPRESSION) {...}
        case IF_CONDITIONAL:
        {
            if (exp_has_correct_semantics(sem_analyser, node->ast_data.exp))
                return false;

            sem_analyser->nesting_lvl++;

            if (!AST_list_has_consistent_semantics(sem_analyser, node->body))
                return false;
            
            sem_analyser->nesting_lvl--;

            break;
        }

        //I.E else if(EXPRESSION) {...}
        case ELSE_IF_CONDITIONAL:
        {
            if (node->prev != IF_CONDITIONAL)
            {
                // TODO -> BAD SYNTAX

                return false;
            }

            if (!exp_has_correct_semantics(sem_analyser, node->ast_data.exp))
                return false;

            sem_analyser->nesting_lvl++;

            if (!AST_list_has_consistent_semantics(sem_analyser, node->body))
                return false;
                
            sem_analyser->nesting_lvl--;
            
            break;
        }

        case ELSE_CONDITIONAL:
        {
            if (node->prev != IF_CONDITIONAL)
            {
                // TODO -> BAD SYNTAX

                return false;
            }

            sem_analyser->nesting_lvl++;

            if (!AST_list_has_consistent_semantics(sem_analyser, node->body))
                return false;

            sem_analyser->nesting_lvl--;

            break;
        }
        case WHILE_LOOP:
        {
            if (!exp_has_correct_semantics(sem_analyser, node->ast_data.exp))
                return false;

            sem_analyser->nesting_lvl++;
            bool current_loop_cond = sem_analyser->is_in_loop;

            sem_analyser->is_in_loop = true;
            bool tmp = AST_list_has_consistent_semantics(sem_analyser, node->body);
            sem_analyser->is_in_loop = current_loop_cond;

            sem_analyser->nesting_lvl--;

            if (!tmp)
                return false;

            break;
        }
        
        // i.e func function(EXPRESSION COMPONENT, ... ) {...}
        case FUNCTION_DECLARATION:
        {
            if (!function_declaration_semantics(sem_analyser, node))
                return false;

            sem_analyser->nesting_lvl++;
            Scope this_scope = sem_analyser->scope_type;

            sem_analyser->scope_type=FUNCTION_SCOPE;
            bool tmp = AST_list_has_consistent_semantics(sem_analyser, node->body);
            
            sem_analyser->scope_type=this_scope;
            sem_analyser->nesting_lvl--;
            

            if (!tmp)
                return false;

            break;
        }
        
        // i.e return EXPRESSION ...;
        case RETURN_VAL:
        {
            if (!exp_has_correct_semantics(sem_analyser, node->ast_data.exp))
            {
                return false;
            }
            break;
        }

        // i.e break;
        case LOOP_TERMINATOR:
        {
            if(!sem_analyser->is_in_loop) {
                // TODO: print error message (invalid break statement)
                return false;
            }
            break;

        }

        // i.e continue;
        case LOOP_CONTINUATION:
        {
            if(!sem_analyser->is_in_loop) {
                // TODO: print error message (invalid continue statement)
                return false;
            }
            break;
        }

        // todo
        case EXPRESSION_COMPONENT:
        {

        }

        // todo
        case INLINE_FUNCTION_DECLARATION:
        {
            break;
        }
        }

        node = node->next;
    }

    remove_all_syms_above_nesting_lvl(sem_analyser->symtable, sem_analyser->nesting_lvl);

    return true;
}