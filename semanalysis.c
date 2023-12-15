#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "semanalysis.h"
#include "lexer.h"
#include "errors.h"

/* 
DESCRIPTION:
This file contains the implementation of the Semantic Analyzer 
It checks for the following:
- All referenced variables were declared and exist
- If, else if, and while block contain NON empty expression
- If, else if, else blocks are ordered properly 
(i.e else if block must be preceded by if block, and else block must be preceded by else or else if block)
- Expression Trees contain valid leaf nodes (expression components)
- continue, break must be used witin loop blocks
- Object Declarations must ONLY contain variables, functions, or other object declarations
*/

static bool is_access_modifier_valid(SemanticAnalyzer *sem_analyzer, AST_node *node);
static bool is_exp_component_terminal(enum expression_component_type type);
static bool is_obj_block_valid(SemanticAnalyzer *sem_anaylzer, AST_node *node);
static void add_argument_declarations_to_symtable(SemanticAnalyzer *sem_analyzer, AST_node *node);
static void forward_declare_obj_func(SemanticAnalyzer *sem_analyzer, AST_List *ast_list);
static ExpressionComponent *get_left_most_component(ExpressionComponent *component);

/* Checks that if access modifiers are used in the proper scope */
static bool is_access_modifier_valid(SemanticAnalyzer *sem_analyzer, AST_node *node)
{
    assert(node->access != DOES_NOT_APPLY);
    assert(node->type == VAR_DECLARATION ||
           node->type == FUNCTION_DECLARATION ||
           node->type == OBJECT_DECLARATION);

    // global variables must be defined in the global scope with top level nesting
    if (node->access == GLOBAL_ACCESS &&
        (sem_analyzer->scope_type != GLOBAL_SCOPE || sem_analyzer->nesting_lvl > 0))
    {
        print_invalid_access_modifier_semantics_err(
            sem_analyzer, node->token_num,
            sem_analyzer->scope_type == OBJECT_SCOPE     ? "Global access modifiers can NOT be used in the Object scope."
                                                           "\nThey can only be used in the global scope."
            : sem_analyzer->scope_type == FUNCTION_SCOPE ? "Global access modifiers cant NOT be used in the function scope.\n"
                                                           "They can only be used in the global scope."
                                                         : NULL);
        return false;
    }
    // private variables can only be defined within an object
    if (node->access == PRIVATE_ACCESS && sem_analyzer->scope_type != OBJECT_SCOPE)
    {
        print_invalid_access_modifier_semantics_err(
            sem_analyzer, node->token_num,
            sem_analyzer->scope_type == FUNCTION_SCOPE  ? "Private access modifiers can NOT be used in the function scope."
                                                          "\nThey can only be used in the object scope."
            : sem_analyzer->scope_type == GLOBAL_ACCESS ? "Private access modifiers cant NOT be used in the global scope.\n"
                                                          "They can only be used in the object     scope."
                                                        : NULL);
        return false;
    }

    return true;
}

/* Helper for getting the left most component in the expression component */
static ExpressionComponent *get_left_most_component(ExpressionComponent *component)
{
    while (component->sub_component)
    {
        component = component->sub_component;
    }
    return component;
}

/* Checks if the expression component type is terminal component */
static bool is_exp_component_terminal(enum expression_component_type type)
{
    return type == LIST_CONSTANT ||
           type == STRING_CONSTANT ||
           type == NUMERIC_CONSTANT ||
           type == INLINE_FUNC ||
           type == NULL_CONSTANT ||
           type == HASHMAP_CONSTANT ||
           type == HASHSET_CONSTANT;
}

/* Checks that OBJECT block only contains variable, function, or object declarations */
static bool is_obj_block_valid(SemanticAnalyzer *sem_anaylzer, AST_node *node)
{
    assert(node->type == OBJECT_DECLARATION);
    AST_node *head = node->body->head;
    while (head)
    {
        if (head->type != VAR_DECLARATION &&
            head->type != FUNCTION_DECLARATION &&
            head->type != OBJECT_DECLARATION)
        {
            print_invalid_object_block_err(
                sem_anaylzer, head->token_num,
                "Only Variable, Function, and Object declarations can be used in a top level Object scope.\n"
                "Object body does not run code, it holds declarations.");
            return false;
        }
        head = head->next;
    }
    return true;
}

/* Adds a function scopes parameters variables to symbol table */
static void add_argument_declarations_to_symtable(SemanticAnalyzer *sem_analyzer, AST_node *node)
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

        add_var_to_vartable(
            sem_analyzer->symtable,
            args[i]->component->meta_data.variable_reference,
            sem_analyzer->filename,
            sem_analyzer->nesting_lvl,
            SYMBOL_TYPE_VARIABLE);
    }
}

/* Mallocs semantic analyzer struct */
SemanticAnalyzer *malloc_semantic_analyser(const char *filename, char **lines, int line_num, TokenList *list)
{
    SemanticAnalyzer *sem_analyser = malloc(sizeof(SemanticAnalyzer));
    sem_analyser->is_in_loop = false;
    sem_analyser->nesting_lvl = 0;
    sem_analyser->scope_type = GLOBAL_SCOPE;
    sem_analyser->filename = malloc_string_cpy(NULL, filename);
    sem_analyser->symtable = malloc_symbol_table();
    sem_analyser->lines.line_count = line_num;
    sem_analyser->lines.lines = cpy_2D_string_arr(lines, line_num);
    sem_analyser->token_list = cpy_token_list(list);
    return sem_analyser;
}

/* Frees semantic analyzer */
void free_semantic_analyser(SemanticAnalyzer *sem_analyser)
{
    free_var_table(sem_analyser->symtable);
    free(sem_analyser->filename);

    for (int i = 0; i < sem_analyser->lines.line_count; i++)
    {
        free(sem_analyser->lines.lines[i]);
    }

    free(sem_analyser->lines.lines);
    free_token_list(sem_analyser->token_list);
    free(sem_analyser);
}

/* Checks that the expression as consistent identifiers */
bool exp_has_correct_semantics(SemanticAnalyzer *sem_analyser, ExpressionNode *root)
{
    // Bases cases
    if (!root)
        return true;

    if (root->type == VALUE)
    {
        if (!expression_component_has_correct_semantics(sem_analyser, root->component))
            // function should print out an error
            return false;

        return true;
    }

    // recursive calls
    return exp_has_correct_semantics(sem_analyser, root->LHS) &&
           exp_has_correct_semantics(sem_analyser, root->RHS);
}

// TODO
/* Makes sure expression component is semantically correct */
/*
For each expression component,
it only checks its sub component is semantically valid for the respective component

*/
bool expression_component_has_correct_semantics(SemanticAnalyzer *sem_analyzer, ExpressionComponent *node)
{
    while (node->sub_component)
        node = node->sub_component;

    while (node)
    {
        switch (node->type)
        {

        /*
        Numeric Constant:
        - This a terminal component
        It cannot be a child component (i.e parent->123 is invalid)
        */
        case NUMERIC_CONSTANT:
        {
            if (node->sub_component)
            {
                print_invalid_terminal_top_component_err(
                    sem_analyzer, node,
                    "Proper Syntax: 123123 -> [EXPRESSION COMPONENT]\n");
                return false;
            }

            break;
        }

        /*
        String Constant:
        - This a terminal component
        It cannot be a child component (i.e parent->"string" is invalid)
        */
        case STRING_CONSTANT:
        {
            if (node->sub_component)
            {
                print_invalid_terminal_top_component_err(
                    sem_analyzer, node,
                    "Proper Syntax: \"string\" -> [EXPRESSION COMPONENT]\n");
                return false;
            }

            break;
        }

        /*
        List Constant:
        - This a terminal component
        It cannot be a child component (i.e parent->[...] is invalid)
        */
        case LIST_CONSTANT:
        {
            if (node->sub_component)
            {
                print_invalid_terminal_top_component_err(
                    sem_analyzer, node,
                    "Proper Syntax: [ ... ] -> [EXPRESSION COMPONENT]");
                return false;
            }

            break;
        }

        /*
        Null constant ('null' keyword):
        - This is a terminal component
        Which means that if its used it must at the root of the expression component (i.e parent -> null is invalid)
        */
        case NULL_CONSTANT:
        {
            if (node->sub_component)
            {
                print_invalid_terminal_top_component_err(
                    sem_analyzer, node,
                    "Proper Syntax: null -> [EXPRESSION COMPONENT]");
                return false;
            }

            break;
        }

        /*
        HashMap constant:
        - This a terminal component
        */
        case HASHMAP_CONSTANT:
        {
            if (node->sub_component)
            {
                print_invalid_terminal_top_component_err(
                    sem_analyzer, node,
                    "Proper Syntax: map { v1: e1, ... } -> [EXPRESSION COMPONENT]");
                return false;
            }

            for (int i = 0; i < node->meta_data.HashMap.size; i++)
            {
                if (!exp_has_correct_semantics(sem_analyzer, node->meta_data.HashMap.pairs[i]->key))
                {
                    return false;
                }
                if (!exp_has_correct_semantics(sem_analyzer, node->meta_data.HashMap.pairs[i]->value))
                {
                    return false;
                }
            }

            break;
        }

        /*
        HashSet constant:
        - This is terminal component
        */
        case HASHSET_CONSTANT:
        {
            if (node->sub_component)
            {
                print_invalid_terminal_top_component_err(
                    sem_analyzer, node,
                    "Proper Syntax: set { v1: e1, ... } -> [EXPRESSION COMPONENT]");
                return false;
            }

            for (int i = 0; i < node->meta_data.HashSet.size; i++)
            {
                if (!exp_has_correct_semantics(sem_analyzer, node->meta_data.HashSet.values[i]))
                {
                    return false;
                }
            }

            break;
        }

        /*
        Variable:
        - If the variable is at the root of the expression component.
        Its identifer must have been declared somewhere in the current or higher scope.
        */
        case VARIABLE:
        {
            if (!node->sub_component &&
                !vartable_has_var(sem_analyzer->symtable, node->meta_data.variable_reference))
            {
                print_undeclared_identifier_err(sem_analyzer, node, NULL);
                return false;
            }

            break;
        }

        /*
        Indexes:
        - Sub Expression must be valid
        - Indexes are allowed to have list and string constants as sub components
        since it makes sense for list and strings (i.e fetching the nth list element or character)
        */
        case LIST_INDEX:
        {
            if (!node->meta_data.list_index)
            {
                print_empty_exp_err(sem_analyzer, node->token_num, "List Indexes must have non empty expressions");
                return false;
            }

            if (node->sub_component &&
                node->sub_component->type != LIST_CONSTANT &&
                node->sub_component->type != STRING_CONSTANT &&
                is_exp_component_terminal(node->sub_component->type))
            {
                print_invalid_index_err(sem_analyzer, node, node->token_num, NULL);
                return false;
            }

            if (!exp_has_correct_semantics(sem_analyzer, node->meta_data.list_index))
                // function should print out an error
                return false;

            break;
        }

        /*
        Function Calls:
        - Must have single standalone identifiers as arguments
        - They are allowed to have inline functions as sub components since the latter is still a function
        - Its arguments must be valid expressions
        */
        case FUNC_CALL:
        {
            // if (node->sub_component &&
            //     node->sub_component->type == INLINE_FUNC &&
            //     is_exp_component_terminal(node->sub_component->type))
            // {
            //     print_invalid_func_call_err(sem_analyzer, node, node->token_num, NULL);
            //     return false;
            // }
            if (node->sub_component &&
                node->sub_component->type == INLINE_FUNC &&
                node->sub_component->meta_data.inline_func->ast_data.func_args.args_num !=
                    node->meta_data.func_data.args_num)
            {
                print_invalid_arg_count_err(
                    sem_analyzer,
                    node->meta_data.func_data.args_num,
                    node->sub_component->meta_data.inline_func->ast_data.func_args.args_num,
                    node->token_num, NULL);
                return false;
            }

            for (int i = 0; i < node->meta_data.func_data.args_num; i++)
            {
                if (!exp_has_correct_semantics(sem_analyzer, node->meta_data.func_data.func_args[i]))
                {
                    // function will print out error
                    return false;
                }
            }

            break;
        }

        /*
        Inline functions (or nameless functions):
        - This a terminal component
        It cannot be a child component (i.e parent -> func(...) {...} is invalid)
        - Its arguments must be single standalone identifers
        */
        case INLINE_FUNC:
        {
            if (node->sub_component)
            {
                print_invalid_terminal_top_component_err(
                    sem_analyzer, node,
                    "Proper Syntax: func (...) { ... } -> [EXPRESSION COMPONENT]");
                return false;
            }

            if (!check_argument_semantics(sem_analyzer, node->meta_data.inline_func))
            {
                // function will print out error
                return false;
            }

            Scope current_scope = sem_analyzer->scope_type;
            sem_analyzer->nesting_lvl++;
            sem_analyzer->scope_type = FUNCTION_SCOPE;

            add_argument_declarations_to_symtable(sem_analyzer, node->meta_data.inline_func);

            if (!AST_list_has_consistent_semantics(sem_analyzer, node->meta_data.inline_func->body))
                // function should print an error
                return false;

            remove_all_vars_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

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

/* Checks that var assignment makes sense */
bool var_assignment_has_correct_semantics(SemanticAnalyzer *sem_analyser, AST_node *node)
{
    assert(node->type == VAR_ASSIGNMENT);
    ExpressionComponent *exp_node = node->identifier.expression_component;

    if (is_exp_component_terminal(exp_node->type))
    {
        char *msg = NULL;
        switch (exp_node->type)
        {
        case LIST_CONSTANT:
            msg = "Cannot assign a List Constant.";
            break;
        case STRING_CONSTANT:
            msg = "Cannot assign a String Constant.";
            break;
        case NUMERIC_CONSTANT:
            msg = "Cannot assign a Number Constant.";
            break;
        case INLINE_FUNC:
            msg = "Cannot assign a Inline Function.";
            break;
        case NULL_CONSTANT:
            msg = "Cannot assign a Null value.";
            break;
        case HASHMAP_CONSTANT:
            msg = "Cannot assign a Map Constant.";
            break;
        case HASHSET_CONSTANT:
            msg = "Cannot assign a Set Constant.";
            break;
        default:
            break;
        }
        print_invalid_var_assignment_err(sem_analyser, exp_node->token_num, msg);
        return false;
    }

    return expression_component_has_correct_semantics(sem_analyser, exp_node) &&
           exp_has_correct_semantics(sem_analyser, node->ast_data.exp);
}

/* Checks argument semantics (and to some extent syntax) i.e [IDENTIFER]([EXP NODE],[EXP NODE],...)*/
bool check_argument_semantics(SemanticAnalyzer *sem_analyser, AST_node *node)
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

            print_invalid_arg_identifier_err(
                sem_analyser, args[i]->token_num,
                node->type == FUNCTION_DECLARATION          ? "Function Declarations arguments must be standalone identifiers\n"
                                                              "Proper Syntax: func function (arg1,arg2, ... ) { ... }"
                : node->type == OBJECT_DECLARATION          ? "Object Declarations arguments must be standalone identifiers\n"
                                                              "Proper Syntax: object function (arg1,arg2, ... ) { ... }"
                : node->type == INLINE_FUNCTION_DECLARATION ? "Inline Function Declarations arguments must be standalone identifiers\n"
                                                              "Proper Syntax: func (arg1,arg2, ... ) { ... }"
                                                            : NULL);

            return false;
        }
    }

    return true;
}

/* Iterates through AST and adds object and function declarations to symtable */
static void forward_declare_obj_func(SemanticAnalyzer *sem_analyzer, AST_List *ast_list)
{
    if (!ast_list)
        return;

    AST_node *node = ast_list->head;
    while (node)
    {
        switch (node->type)
        {
        case OBJECT_DECLARATION:
            add_var_to_vartable(
                sem_analyzer->symtable,
                node->identifier.func_name,
                sem_analyzer->filename,
                sem_analyzer->nesting_lvl,
                SYMBOL_TYPE_OBJECT);
            break;
        case FUNCTION_DECLARATION:
            add_var_to_vartable(
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
bool AST_list_has_consistent_semantics(SemanticAnalyzer *sem_analyzer, AST_List *ast_list)
{
    // Empty code block are valid
    if (!ast_list)
        return true;

    AST_node *node = ast_list->head;

    // forward declares function, object declarations
    // Allows for recursive/ recursively dependant definitions
    // forward_declare_obj_func(sem_analyzer, ast_list);

    while (node)
    {
        switch (node->type)
        {

        /*
        Variable Declarations:
        - let keyword can be preceded by a semantically valid access modifier
        - Initial assignment can be empty, but if its not it must a valid expression
        - Identifer is put into variable table
        */
        case VAR_DECLARATION:
        {
            // access modifier semantics
            if (!is_access_modifier_valid(sem_analyzer, node))
            {
                // function prints out error
                return false;
            }

            // checks assignment value
            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
            {
                // function prints out error
                return false;
            }

            // saves symbol
            add_var_to_vartable(
                sem_analyzer->symtable,
                node->identifier.declared_var,
                sem_analyzer->filename,
                sem_analyzer->nesting_lvl,
                SYMBOL_TYPE_VARIABLE);

            break;
        }

        /*
        Variable assignment:
        - Assignment variable must have been declared in the current or higher scope
        - Assignment expression must also be valid
        */
        case VAR_ASSIGNMENT:
        {
            if (!var_assignment_has_correct_semantics(sem_analyzer, node))
            {
                // function prints out error
                return false;
            }

            break;
        }

        /*
        If block:
        - Conditional expression must be valid
        - And its sub block must also be valid
        */
        case IF_CONDITIONAL:
        {
            // checks if conditional expression is empty
            if (!node->ast_data.exp)
            {
                print_empty_exp_err(sem_analyzer, node->token_num,
                                    "Proper Syntax: if ( expression ...) { ... }");
                return false;
            }

            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
            {
                // function prints out error
                return false;
            }

            sem_analyzer->nesting_lvl++;

            if (!AST_list_has_consistent_semantics(sem_analyzer, node->body))
            {
                // function prints out error
                return false;
            }

            sem_analyzer->nesting_lvl--;

            break;
        }

        /*
        else if block:
        - Must be preceded by an if OR else if block (which must be valid)
        - Conditional expression must be valid
        - Sub block must be valid
        */
        case ELSE_IF_CONDITIONAL:
        {
            // checks if conditional expression is empty
            if (!node->ast_data.exp)
            {
                print_empty_exp_err(sem_analyzer, node->token_num,
                                    "Proper Syntax: ... else if ( expression ... ) { ... }");
                return false;
            }

            if (!node->prev || (node->prev &&
                                node->prev->type != IF_CONDITIONAL &&
                                node->prev->type != ELSE_IF_CONDITIONAL))
            {

                print_invalid_else_if_block_err(sem_analyzer, node, node->token_num,
                                                "Proper Syntax: if (...) { ... } else if (...) { ... } ... else if (...) { ... }");
                return false;
            }

            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
                // function prints out error
                return false;

            sem_analyzer->nesting_lvl++;

            if (!AST_list_has_consistent_semantics(sem_analyzer, node->body))
                // function prints out error
                return false;

            sem_analyzer->nesting_lvl--;

            break;
        }

        /*
        else block:
        - Must be preceded by a if or else if block (which must be valid)
        - Sub block must be valid
        */
        case ELSE_CONDITIONAL:
        {
            if (!node->prev || (node->prev &&
                                node->prev->type != IF_CONDITIONAL &&
                                node->prev->type != ELSE_IF_CONDITIONAL))
            {
                print_invalid_else_block_err(
                    sem_analyzer, node, node->token_num,
                    "Proper Syntax: if (...) { ... } else if (...) { ... } ... else { ... }");
                return false;
            }

            sem_analyzer->nesting_lvl++;

            if (!AST_list_has_consistent_semantics(sem_analyzer, node->body))
            {
                // function prints out error
                return false;
            }

            sem_analyzer->nesting_lvl--;

            break;
        }
        /*
        while block:
        - Conditional expression must be valid
        - Sub block must be valid
        */
        case WHILE_LOOP:
        {
            // checks if conditional expression is empty
            if (!node->ast_data.exp)
            {
                print_empty_exp_err(sem_analyzer, node->token_num,
                                    "Proper Syntax: while ( expression ...) { ... }");
                return false;
            }

            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
            {
                // function prints out error
                return false;
            }

            sem_analyzer->nesting_lvl++;
            bool current_loop_cond = sem_analyzer->is_in_loop;

            sem_analyzer->is_in_loop = true;

            bool tmp = AST_list_has_consistent_semantics(sem_analyzer, node->body);
            // if tmp false, an error would have been printed out

            sem_analyzer->is_in_loop = current_loop_cond;
            sem_analyzer->nesting_lvl--;

            if (!tmp)
                return false;

            break;
        }

        /*
        Function Declarations:
        - Access modifier must be semantically valid
        - Function arguments must be standalone identifiers
        - Sub block must be valid
        - Function name will have been forward declared
        - Argument names are added to the functions scope
        */
        case FUNCTION_DECLARATION:
        {
            if (!is_access_modifier_valid(sem_analyzer, node))
                // function prints out error
                return false;

            if (!check_argument_semantics(sem_analyzer, node))
                // function prints out error
                return false;

            add_var_to_vartable(
                sem_analyzer->symtable,
                node->identifier.func_name,
                sem_analyzer->filename,
                sem_analyzer->nesting_lvl,
                SYMBOL_TYPE_FUNCTION);

            sem_analyzer->nesting_lvl++;
            Scope this_scope = sem_analyzer->scope_type;

            add_argument_declarations_to_symtable(sem_analyzer, node);

            sem_analyzer->scope_type = FUNCTION_SCOPE;
            bool tmp = AST_list_has_consistent_semantics(sem_analyzer, node->body);
            // if tmp false, an error should have been printed out

            remove_all_vars_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

            sem_analyzer->scope_type = this_scope;
            sem_analyzer->nesting_lvl--;

            if (!tmp)
                return false;

            break;
        }

        /*
        Object Declarations:
        - Access modifer must be semantically valid
        - Object arguments must be standalone identifers
        - Object name is added to var table (however it should have been already forward declared)
        - Object Arguments are added to var table for the object's lower scope
        - Sub block must only contain variable, function, and/or object declarations
        */
        case OBJECT_DECLARATION:
        {
            if (!is_access_modifier_valid(sem_analyzer, node))
                // function prints out error
                return false;

            if (!check_argument_semantics(sem_analyzer, node))
                // function prints out error
                return false;

            // adds object name has symbol to table
            add_var_to_vartable(
                sem_analyzer->symtable,
                node->identifier.obj_name,
                sem_analyzer->filename,
                sem_analyzer->nesting_lvl,
                SYMBOL_TYPE_OBJECT);

            if (!is_obj_block_valid(sem_analyzer, node))
                // function prints out error
                return false;

            sem_analyzer->nesting_lvl++;
            Scope this_scope = sem_analyzer->scope_type;

            add_argument_declarations_to_symtable(sem_analyzer, node);

            sem_analyzer->scope_type = OBJECT_SCOPE;

            bool tmp = AST_list_has_consistent_semantics(sem_analyzer, node->body);
            // if tmp false, an error should have been printed out

            remove_all_vars_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

            sem_analyzer->scope_type = this_scope;
            sem_analyzer->nesting_lvl--;

            if (!tmp)
                return false;

            break;
        }

        /*
        Inline function:
        - Arguments must be standalone identifiers
        - Arguments are added to var table for the functions lower scope
        - Sub block must be valid
        */
        case INLINE_FUNCTION_DECLARATION:
        {
            if (!AST_list_has_consistent_semantics(sem_analyzer, node->body))
                // function will print an error
                return false;

            sem_analyzer->nesting_lvl++;
            Scope this_scope = sem_analyzer->scope_type;

            add_argument_declarations_to_symtable(sem_analyzer, node);

            sem_analyzer->scope_type = FUNCTION_SCOPE;
            bool tmp = AST_list_has_consistent_semantics(sem_analyzer, node->body);

            remove_all_vars_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

            sem_analyzer->scope_type = this_scope;
            sem_analyzer->nesting_lvl--;

            if (!tmp)
                return false;

            break;
        }

        /*
        Return value:
        - return expression must be valid
        - return values can be used in the global scope (meaning end of the program)
        */
        case RETURN_VAL:
        {
            if (!exp_has_correct_semantics(sem_analyzer, node->ast_data.exp))
                // function will print out an error
                return false;

            break;
        }

        /*
        Loop termination (break keyword)
        - Its a standalone keyword (i.e used by itself)
        - Can only be used while in a loop block
        */
        case LOOP_TERMINATOR:
        {
            if (!sem_analyzer->is_in_loop)
            {
                // TODO: print error message (invalid break statement)
                return false;
            }
            break;
        }

        /*
        Loop continuation (continue keyword)
        - Its a standalone keyword (i.e used by itself)
        - Can only be used while in a loop block
        */
        case LOOP_CONTINUATION:
        {
            if (!sem_analyzer->is_in_loop)
            {
                // TODO: print error message (invalid continue statement)
                return false;
            }
            break;
        }

        /*
        Expression component:
        - Represents a standalone expression component (can represent standalone function calls)
        - Expression component must be valid
        */
        case EXPRESSION_COMPONENT:
        {
            if (!expression_component_has_correct_semantics(sem_analyzer, node->identifier.expression_component))
            {
                // Invalid Syntax for expression component
                // function will print out an error
                return false;
            }
            break;
        }
        }

        node = node->next;
    }

    // we only clear symtable if its nested, top level declarations are kept
    if (sem_analyzer->nesting_lvl > 0)
        remove_all_vars_above_nesting_lvl(sem_analyzer->symtable, sem_analyzer->nesting_lvl);

    return true;
}
