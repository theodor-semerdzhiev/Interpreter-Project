#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "semanalysis.h"
#include "lexer.h"

static bool is_access_modifier_valid(SemanticAnalyser *sem_analyzer, AST_node *node)
{
    assert(node->access != DOES_NOT_APPLY);
    assert(node->type == VAR_DECLARATION ||
           node->type == FUNCTION_DECLARATION ||
           node->type == OBJECT_DECLARATION);

    // global variables must be defined in the global scope with top level nesting
    if (node->access == GLOBAL_ACCESS &&
        (sem_analyzer->scope_type != GLOBAL_SCOPE || sem_analyzer->nesting_lvl > 0))
    {
        return false;
    }
    // private variables can only be defined within an object
    if (node->access == PRIVATE_ACCESS && sem_analyzer->scope_type != OBJECT_SCOPE)
    {
        return false;
    }

    return true;
}

/* Checks if the expression component type is terminal component */
static bool is_exp_component_terminal(enum expression_component_type type)
{
    return type == LIST_CONSTANT ||
           type == STRING_CONSTANT ||
           type == NUMERIC_CONSTANT ||
           type == INLINE_FUNC ||
           type == NULL_CONSTANT;
}

/* Checks that OBJECT block only contains variable, function, or object declarations */
static bool is_obj_block_valid(SemanticAnalyser *sem_anaylzer, AST_node *node)
{
    assert(node->type == OBJECT_DECLARATION);
    AST_node *head = node->body->head;
    while (head)
    {
        if (head->type != VAR_DECLARATION &&
            head->type != FUNCTION_DECLARATION &&
            head->type != OBJECT_DECLARATION)
        {
            return false;
        }
        head = head->next;
    }
    return true;
}

/* Adds a function scopes parameters variables to symbol table */
static void add_argument_declarations_to_symtable(SemanticAnalyser *sem_analyzer, AST_node *node)
{
    assert(node->type == FUNCTION_DECLARATION ||
           node->type == INLINE_FUNCTION_DECLARATION ||
           node->type == OBJECT_DECLARATION);

    int args_count;
    ExpressionNode **args;

    if (node->type == FUNCTION_DECLARATION || node->type == INLINE_FUNCTION_DECLARATION)
    {
        args_count = node->ast_data.func_args.args_num;
        args = node->ast_data.func_args.func_prototype_args;
    }
    else
    {
        args_count = node->ast_data.obj_args.args_num;
        args = node->ast_data.obj_args.object_prototype_args;
    }

    for (int i = 0; i < args_count; i++)
    {
        assert(args[i]->component->meta_data.variable_reference);

        add_sym_to_symtable(
            sem_analyzer->symtable,
            args[i]->component->meta_data.variable_reference,
            sem_analyzer->filename,
            sem_analyzer->nesting_lvl,
            SYMBOL_TYPE_VARIABLE);
    }
}

/* Mallocs semantic analyser struct */
SemanticAnalyser *malloc_semantic_analyser(const char *filename, const char **lines, const int line_num)
{
    SemanticAnalyser *sem_analyser = malloc(sizeof(SemanticAnalyser));
    sem_analyser->is_in_loop = false;
    sem_analyser->nesting_lvl = 0;
    sem_analyser->scope_type = GLOBAL_SCOPE;
    sem_analyser->filename = malloc_string_cpy(NULL, filename);
    sem_analyser->symtable = malloc_symbol_table();
    sem_analyser->lines.line_count = line_num;
    sem_analyser->lines.lines = cpy_2D_string_arr(lines, line_num);
    return sem_analyser;
}

/* Frees semantic analyser */
void free_semantic_analyser(SemanticAnalyser *sem_analyser)
{
    free_sym_table(sem_analyser->symtable);
    free(sem_analyser->filename);

    for(int i=0; i < sem_analyser->lines.line_count; i++) {
        free(sem_analyser->lines.lines[i]);
    }

    free(sem_analyser->lines.lines);
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
    return exp_has_correct_semantics(sem_analyser, root->LHS) &&
           exp_has_correct_semantics(sem_analyser, root->RHS);
}

// TODO
/* Makes sure expression component is semantically correct */
bool expression_component_has_correct_semantics(SemanticAnalyser *sem_analyzer, ExpressionComponent *node)
{
    while (node->sub_component)
        node = node->sub_component;

    while (node)
    {
        switch (node->type)
        {
        // terminal component
        case NUMERIC_CONSTANT:
        {
            if (node->top_component || node->sub_component)
            {
                // Invalid Syntax: 200->[EXPRESSION COMPONENT] | [EXPRESSION COMPONENT]->200, not valid
                return false;
            }

            break;
        }
        // terminal component
        case STRING_CONSTANT:
        {
            if (node->top_component || node->sub_component)
            {
                // Invalid Syntax: "123123"->[EXPRESSION COMPONENT] | [EXPRESSION COMPONENT]->"123123" not valid
                return false;
            }

            break;
        }
        // terminal component
        case LIST_CONSTANT:
        {
            if (node->top_component &&
                (node->top_component->type != IDENTIFIER || node->top_component->type != LIST_INDEX))
            {
                // Invalid Syntax: [1,2,3,4,5,6,7,8]()
                return false;
            }

            break;
        }

        // terminal component
        case NULL_CONSTANT:
        {
            if (node->top_component &&
                (node->top_component->type != IDENTIFIER || node->top_component->type != LIST_INDEX))
            {
                // Invalid Syntax: null->identifier...
                return false;
            }

            break;
        }

        case VARIABLE:
        {
            if (node->top_component && is_exp_component_terminal(node->top_component->type))
            {
                // Invalid Syntax: num -> [TERMINAL EXPRESSION COMPONENT]

                return false;
            }
            if (!node->sub_component &&
                !symtable_has_sym(sem_analyzer->symtable, node->meta_data.variable_reference))
            {

                // num -> ... (num must be a defined visible variable)
                return false;
            }

            break;
        }

        case LIST_INDEX:
        {
            if (node->top_component && is_exp_component_terminal(node->top_component->type))
            {
                // Invalid Syntax: [EXPRESSION COMPONENT][LIST_INDEX] -> [TERMINAL EXPRESSION COMPONENT]
                return false;
            }

            // Makes sure that
            if (!exp_has_correct_semantics(sem_analyzer, node->meta_data.list_index))
                return false;

            break;
        }

        case FUNC_CALL:
        {
            if (node->top_component && is_exp_component_terminal(node->top_component->type))
            {
                // Invalid Syntax: [EXPRESSION COMPONENT](ARGS,...) -> [TERMINAL EXPRESSION COMPONENT]
                return false;
            }

            for (int i = 0; i < node->meta_data.func_data.args_num; i++)
            {
                if (!exp_has_correct_semantics(sem_analyzer, node->meta_data.func_data.func_args[i]))
                {
                    // Invalid Syntax: [EXPRESSION COMPONENT]([EXPRESSION TREE, ..., EXPRESSION TREE])
                    return false;
                }
            }

            break;
        }

        // TODO
        case INLINE_FUNC:
        {
            if (node->top_component && is_exp_component_terminal(node->top_component->type))
            {
                // Invalid Syntax: [EXPRESSION COMPONENT](ARGS,...) -> [TERMINAL EXPRESSION COMPONENT]
                return false;
            }

            if (!check_argument_semantics(sem_analyzer, node->meta_data.inline_func))
            {
                // Invalid Syntax: [EXPRESSION COMPONENT]([EXPRESSION TREE, ..., EXPRESSION TREE])
                return false;
            }

            Scope current_scope = sem_analyzer->scope_type;
            sem_analyzer->nesting_lvl++;
            sem_analyzer->scope_type = FUNCTION_SCOPE;

            add_argument_declarations_to_symtable(sem_analyzer, node->meta_data.inline_func);

            if (!AST_list_has_consistent_semantics(sem_analyzer, node->meta_data.inline_func->body))
            {
                return false;
            }

            remove_all_syms_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

            sem_analyzer->scope_type = current_scope;
            sem_analyzer->nesting_lvl--;

            break;
        }

        default:
            return false;
        }

        node = node->top_component;
    }

    return true;
}

/* Checks that var assignment makes semantic sense */
bool var_assignment_has_correct_semantics(SemanticAnalyser *sem_analyser, AST_node *node)
{
    assert(node->type == VAR_ASSIGNMENT);
    return expression_component_has_correct_semantics(sem_analyser, node->identifier.expression_component) &&
           exp_has_correct_semantics(sem_analyser, node->ast_data.exp);
}

/* Checks argument semantics (and to some extent syntax) i.e [IDENTIFER]([EXP NODE],[EXP NODE],...)*/
bool check_argument_semantics(SemanticAnalyser *sem_analyser, AST_node *node)
{
    assert(
        node->type == FUNCTION_DECLARATION ||
        node->type == INLINE_FUNCTION_DECLARATION ||
        node->type == OBJECT_DECLARATION);

    int args_count;
    ExpressionNode **args;

    if (node->type == FUNCTION_DECLARATION || node->type == INLINE_FUNCTION_DECLARATION)
    {
        args_count = node->ast_data.func_args.args_num;
        args = node->ast_data.func_args.func_prototype_args;
    }
    else
    {
        args_count = node->ast_data.obj_args.args_num;
        args = node->ast_data.obj_args.object_prototype_args;
    }

    for (int i = 0; i < args_count; i++)
    {
        // invalid argument (must be a single variable), i.e func sum(n0,n1)
        if (args[i]->type != VALUE || args[i]->component->type != VARIABLE)
        {
            return false;
        }
    }

    return true;
}

/* Iterates through AST and adds object and function declarations to symtable */
static void forward_declare_obj_func(SemanticAnalyser *sem_analyzer, AST_List *ast_list)
{
    if (!ast_list)
        return;

    AST_node *node = ast_list->head;
    while (node)
    {
        switch (node->type)
        {
        case OBJECT_DECLARATION:
            add_sym_to_symtable(
                sem_analyzer->symtable,
                node->identifier.func_name,
                sem_analyzer->filename,
                sem_analyzer->nesting_lvl,
                SYMBOL_TYPE_OBJECT);
            break;
        case FUNCTION_DECLARATION:
            add_sym_to_symtable(
                sem_analyzer->symtable,
                node->identifier.obj_name,
                sem_analyzer->filename,
                sem_analyzer->nesting_lvl,
                SYMBOL_TYPE_FUNCTION);
            break;

        default:
            break;
        }

        node = node->next;
    }
}

/* Checks that AST list has consistent semantics
- All variable references are valid
- Function declarations are valid
- Program makes logical sense
*/
bool AST_list_has_consistent_semantics(SemanticAnalyser *sem_analyzer, AST_List *ast_list)
{
    if (!ast_list)
        return true;

    AST_node *node = ast_list->head;

    forward_declare_obj_func(sem_analyzer, ast_list);

    while (node)
    {
        switch (node->type)
        {

        // i.e let var = ...
        case VAR_DECLARATION:
        {
            // access modifier semantics
            if (!is_access_modifier_valid(sem_analyzer, node))
            {
                return false;
            }

            // checks assignment value
            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
            {
                return false;
            }

            // saves symbol
            add_sym_to_symtable(
                sem_analyzer->symtable,
                node->identifier.declared_var,
                sem_analyzer->filename,
                sem_analyzer->nesting_lvl,
                SYMBOL_TYPE_VARIABLE);

            break;
        }

        // i.e var = ...
        case VAR_ASSIGNMENT:
        {
            if (!var_assignment_has_correct_semantics(sem_analyzer, node))
                return false;

            break;
        }

        // i.e if(EXPRESSION) {...}
        case IF_CONDITIONAL:
        {
            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
                return false;

            sem_analyzer->nesting_lvl++;

            if (!AST_list_has_consistent_semantics(sem_analyzer, node->body))
                return false;

            sem_analyzer->nesting_lvl--;

            break;
        }

        // I.E else if(EXPRESSION) {...}
        case ELSE_IF_CONDITIONAL:
        {
            if (!node->prev || (node->prev &&
                                node->prev->type != IF_CONDITIONAL &&
                                node->prev->type != ELSE_IF_CONDITIONAL))
            {
                // TODO -> BAD SYNTAX

                return false;
            }

            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
                return false;

            sem_analyzer->nesting_lvl++;

            if (!AST_list_has_consistent_semantics(sem_analyzer, node->body))
                return false;

            sem_analyzer->nesting_lvl--;

            break;
        }

        case ELSE_CONDITIONAL:
        {
            if (!node->prev || (node->prev &&
                                node->prev->type != IF_CONDITIONAL &&
                                node->prev->type != ELSE_IF_CONDITIONAL))
            {
                // TODO -> BAD SYNTAX

                return false;
            }

            sem_analyzer->nesting_lvl++;

            if (!AST_list_has_consistent_semantics(sem_analyzer, node->body))
                return false;

            sem_analyzer->nesting_lvl--;

            break;
        }
        case WHILE_LOOP:
        {
            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
                return false;

            sem_analyzer->nesting_lvl++;
            bool current_loop_cond = sem_analyzer->is_in_loop;

            sem_analyzer->is_in_loop = true;
            bool tmp = AST_list_has_consistent_semantics(sem_analyzer, node->body);

            sem_analyzer->is_in_loop = current_loop_cond;
            sem_analyzer->nesting_lvl--;

            if (!tmp)
                return false;

            break;
        }

        // i.e func function(EXPRESSION COMPONENT, ... ) {...}
        case FUNCTION_DECLARATION:
        {
            // access modifier semantics
            if (!is_access_modifier_valid(sem_analyzer, node))
            {
                return false;
            }

            if (!check_argument_semantics(sem_analyzer, node))
            {
                return false;
            }

            // adds function name has symbol to table
            add_sym_to_symtable(
                sem_analyzer->symtable,
                node->identifier.func_name,
                sem_analyzer->filename,
                sem_analyzer->nesting_lvl,
                SYMBOL_TYPE_FUNCTION);

            sem_analyzer->nesting_lvl++;
            Scope this_scope = sem_analyzer->scope_type;

            // adds func args to symbol table
            add_argument_declarations_to_symtable(sem_analyzer, node);

            sem_analyzer->scope_type = FUNCTION_SCOPE;
            bool tmp = AST_list_has_consistent_semantics(sem_analyzer, node->body);

            remove_all_syms_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

            sem_analyzer->scope_type = this_scope;
            sem_analyzer->nesting_lvl--;

            if (!tmp)
                return false;

            break;
        }

        // TODO
        case OBJECT_DECLARATION:
        {
            // access modifier semantics
            if (!is_access_modifier_valid(sem_analyzer, node))
            {
                return false;
            }

            // adds function name has symbol to table
            add_sym_to_symtable(
                sem_analyzer->symtable,
                node->identifier.obj_name,
                sem_analyzer->filename,
                sem_analyzer->nesting_lvl,
                SYMBOL_TYPE_OBJECT);

            if (!is_obj_block_valid(sem_analyzer, node))
            {
                return false;
            }

            sem_analyzer->nesting_lvl++;
            Scope this_scope = sem_analyzer->scope_type;

            // adds func args to symbol table
            add_argument_declarations_to_symtable(sem_analyzer, node);

            sem_analyzer->scope_type = OBJECT_SCOPE;

            bool tmp = AST_list_has_consistent_semantics(sem_analyzer, node->body);

            remove_all_syms_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

            sem_analyzer->scope_type = this_scope;
            sem_analyzer->nesting_lvl--;

            if (!tmp)
                return false;

            break;
        }

        case INLINE_FUNCTION_DECLARATION:
        {
            if (!AST_list_has_consistent_semantics(sem_analyzer, node->body))
            {
                return false;
            }

            sem_analyzer->nesting_lvl++;
            Scope this_scope = sem_analyzer->scope_type;

            // adds func args to symbol table
            add_argument_declarations_to_symtable(sem_analyzer, node);

            sem_analyzer->scope_type = FUNCTION_SCOPE;
            bool tmp = AST_list_has_consistent_semantics(sem_analyzer, node->body);

            remove_all_syms_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

            sem_analyzer->scope_type = this_scope;
            sem_analyzer->nesting_lvl--;

            if (!tmp)
                return false;

            break;
        }

        // i.e return EXPRESSION ...;
        case RETURN_VAL:
        {
            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
            {
                return false;
            }
            break;
        }

        // i.e break;
        case LOOP_TERMINATOR:
        {
            if (!sem_analyzer->is_in_loop)
            {
                // TODO: print error message (invalid break statement)
                return false;
            }
            break;
        }

        // i.e continue;
        case LOOP_CONTINUATION:
        {
            if (!sem_analyzer->is_in_loop)
            {
                // TODO: print error message (invalid continue statement)
                return false;
            }
            break;
        }

        // todo
        case EXPRESSION_COMPONENT:
        {
            if (!expression_component_has_correct_semantics(sem_analyzer, node->identifier.expression_component))
            {
                // Invalid Syntax for expression component
                return false;
            }
            break;
        }
        }

        node = node->next;
    }

    // we only clear symtable if its nested, top level declarations are kept
    if (sem_analyzer->nesting_lvl > 0)
        remove_all_syms_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

    return true;
}
