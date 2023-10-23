#include <stdlib.h>
#include <stdio.h>
#include "parser.h"
#include "dbgtools.h"

/* Converts AccessModifer to readable string */
static char* access_modifer_to_string(AccessModifier access) {
  switch(access) {
    case PRIVATE_ACCESS:
      return "Private Access";
    case GLOBAL_ACCESS:
      return "Global Access";
    case PUBLIC_ACCESS:
      return "Public Access";
    case DOES_NOT_APPLY:
      return "";
  }
}

/* Prints out lexeme array list */
void print_lexeme_arr_list(TokenList *lexemes)
{
  printf("Length: %zu\n", lexemes->len);

  for (int i = 0; i < (int)lexemes->len; i++)
  {
    char str[2];
    str[0] = lexemes->list[i]->type;
    str[1] = '\0';

    char *type_in_str;

    switch (lexemes->list[i]->type)
    {
    case UNDEFINED:
      type_in_str = "UNDEFINED";
      break;
    case WHITESPACE:
      type_in_str = "WHITESPACE";
      break;
    case HASHTAG:
      type_in_str = "'#'";
      break;
    case DOT:
      type_in_str = "'.'";
      break;
    case SEMI_COLON:
      type_in_str = "';'";
      break;
    case QUOTES:
      type_in_str = "\"";
      break;
    case COMMA:
      type_in_str = "','";
      break;
    case OPEN_CURLY_BRACKETS:
      type_in_str = "'{'";
      break;
    case CLOSING_CURLY_BRACKETS:
      type_in_str = "'}'";
      break;
    case OPEN_PARENTHESIS:
      type_in_str = "'('";
      break;
    case CLOSING_PARENTHESIS:
      type_in_str = "')'";
      break;
    case OPEN_SQUARE_BRACKETS:
      type_in_str = "'['";
      break;
    case CLOSING_SQUARE_BRACKETS:
      type_in_str = "']'";
      break;
    case ASSIGNMENT_OP:
      type_in_str = "'='";
      break;
    case MULT_OP:
      type_in_str = "'*'";
      break;
    case DIV_OP:
      type_in_str = "'/'";
      break;
    case PLUS_OP:
      type_in_str = "'+'";
      break;
    case MINUS_OP:
      type_in_str = "'-'";
      break;
    case MOD_OP:
      type_in_str = "'%'";
      break;
    case SHIFT_LEFT_OP:
      type_in_str = "'<<'";
      break;
    case SHIFT_RIGHT_OP:
      type_in_str = "'>>";
      break;
    case BITWISE_AND_OP:
      type_in_str = "'&";
      break;
    case BITWISE_OR_OP:
      type_in_str = "'|'";
      break;
    case BITWISE_XOR_OP:
      type_in_str = "'^'";
      break;
    case COLON:
      type_in_str = "':'";
      break;
    case ATTRIBUTE_ARROW:
      type_in_str = "'->'";
      break;
    case LOGICAL_AND_OP:
      type_in_str = "'&&'";
      break;
    case LOGICAL_OR_OP:
      type_in_str = "'||'";
      break;
    case LOGICAL_NOT_OP:
      type_in_str = "'!'";
      break;
    case GREATER_THAN_OP:
      type_in_str = "'>'";
      break;
    case LESSER_THAN_OP:
      type_in_str = "'<'";
      break;
    case GREATER_EQUAL_OP:
      type_in_str = "'>='";
      break;
    case LESSER_EQUAL_OP:
      type_in_str = "'<='";
      break;
    case EQUAL_TO_OP:
      type_in_str = "'=='";
      break;
    case END_OF_FILE:
      type_in_str = "END_OF_FILE";
      break;
    case NEW_LINE:
      type_in_str = "NEW_LINE";
      break;
    case KEYWORD:
      type_in_str = "KEYWORD";
      break;
    case STRING_LITERALS:
      type_in_str = "STRING LITERALS";
      break;
    case NUMERIC_LITERAL:
      type_in_str = "NUMERIC_LITERAL";
      break;
    case IDENTIFIER:
      type_in_str = "IDENTIFIER";
      break;
    }
    printf("Index: %d [Line %d] Type: %s     ident:%s\n",
           i,
           lexemes->list[i]->line_num,
           type_in_str,
           lexemes->list[i]->ident);
  }
}

/* Prints a string to stdout some number of times */
static void print_repeated_string(char *str, int repetitions)
{
  for (int i = 0; i < repetitions; i++)
    printf("%s", str);
}

/* Prints out a human readable representation of a expression component */
void print_expression_component(ExpressionComponent *component, char *buffer, int rec_lvl)
{
  if (!component)
    return;

  print_repeated_string(buffer, rec_lvl);

  switch (component->type)
  {
  case VARIABLE:
  {
    printf(" VARIABLE -> %s \n", component->meta_data.variable_reference);
    break;
  }

  case NUMERIC_CONSTANT:
  {
    printf(" NUMERIC_CONSTANT -> %f \n", component->meta_data.numeric_const);
    break;
  }

  case STRING_CONSTANT:
  {
    printf(" STRING_CONSTANT -> \"%s\" \n", component->meta_data.string_literal);
    break;
  }

  case LIST_CONSTANT:
  {
    printf(" LIST_CONSTANT -> \n");
    for (int i = 0; i < component->meta_data.list_const.list_length; i++)
      print_expression_tree(component->meta_data.list_const.list_elements[i], buffer, rec_lvl + 1);

    break;
  }

  case NULL_CONSTANT:
  {
    printf(" NULL_CONSTANT -> null \n");

    break;
  }

  case LIST_INDEX:
  {
    printf(" LIST_INDEX -> \n");
    print_expression_tree(component->meta_data.list_index, buffer, ++rec_lvl);
    break;
  }

  case FUNC_CALL:
  {
    printf(" FUNC_CAL -> Arguments: %s\n", !component->meta_data.func_data.args_num ? "No Args" : "");
    for (int i = 0; i < component->meta_data.func_data.args_num; i++)
      print_expression_tree(component->meta_data.func_data.func_args[i], buffer, rec_lvl + 1);

    break;
  }
  // make sure that it recursively prints out code block
  //
  case INLINE_FUNC:
  {
    printf(" INLINE_FUNC -> Arguments:\n");
    for (int i = 0; i < component->meta_data.func_data.args_num; i++)
      print_expression_tree(component->meta_data.func_data.func_args[i], buffer, rec_lvl + 1);

    print_ast_list(component->meta_data.inline_func->body, buffer, rec_lvl + 1);

    break;
  }

  default:
    printf("Expression Component does not have a valid type \n");
    return;
  }

  if (component->sub_component)
  {
    print_repeated_string(buffer, rec_lvl);
    printf("- Sub Component:\n");
    print_expression_component(component->sub_component, buffer, rec_lvl + 1);
  }
}

/* Prints out a human readable representation of an expression tree */
void print_expression_tree(ExpressionNode *root, char *buffer, int rec_lvl)
{
  print_repeated_string(buffer, rec_lvl);

  if (!root)
  {
    printf("Expression is empty\n");
    return;
  }

  switch (root->type)
  {
  case PLUS:
    printf("- PLUS(+) :\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case MINUS:
    printf("- MINUS(-):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case MULT:
    printf("- MULT(*):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case DIV:
    printf("- DIV(/):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case MOD:
    printf("- MOD(%):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case BITWISE_AND:
    printf("- BITWISE_AND(&):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case BITWISE_OR:
    printf("- BITWISE_OR(|):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case BITWISE_XOR:
    printf("- BITWISE_XOR(^):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case SHIFT_LEFT:
    printf("- SHIFT_LEFT(<<):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case SHIFT_RIGHT:
    printf("- SHIFT_RIGHT(>>):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case GREATER_THAN:
    printf("- GREATER_THAN(>):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;
  case GREATER_EQUAL:
    printf("- GREATER_EQUAL(>=):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case LESSER_THAN:
    printf("- LESSER_THAN(<):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case LESSER_EQUAL:
    printf("- LESSER EQUAL(<=):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case EQUAL_TO:
    printf("- EQUAL_TO(==):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case LOGICAL_AND:
    printf("- LOGICAL_AND(&&):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case LOGICAL_OR:
    printf("- LOGICAL_OR(||):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case LOGICAL_NOT:
    printf("- LOGICAL_NOT(!):\n");
    print_expression_tree(root->LHS, buffer, rec_lvl + 1);
    print_expression_tree(root->RHS, buffer, rec_lvl + 1);
    break;

  case VALUE:
    printf("- VALUE:\n");
    print_expression_component(root->component, buffer, rec_lvl + 1);
    break;

  default:
    printf("Expression Tree type not valid -> %d\n", root->type);
    break;
  }
}

/* Prints out a human readable representation of a abstract syntax node (struct ast_node) to sdtin */
void print_ast_node(AST_node *node, char *buffer, int rec_lvl)
{
  print_repeated_string(buffer, rec_lvl);
  if (!node)
  {
    printf("Code block empty");
    return;
  }

  switch (node->type)
  {
  case VAR_DECLARATION:
  {
    printf("@ VAR_DECLARATION: %s \n", node->identifier.declared_var);
    print_repeated_string(buffer, rec_lvl);
    printf("ACCESS MODIFIER: %s \n", access_modifer_to_string(node->access));
    print_expression_tree(node->ast_data.exp, buffer, rec_lvl + 1);
    break;
  }

  case VAR_ASSIGNMENT:
  {
    printf("@ VAR_ASSIGNMENT: \n");
    print_expression_component(node->identifier.expression_component, buffer, rec_lvl + 1);
    print_expression_tree(node->ast_data.exp, buffer, rec_lvl + 1);
    break;
  }
  case IF_CONDITIONAL:
  {
    printf("@ IF_CONDITIONAL: \n");
    print_expression_tree(node->ast_data.exp, buffer, rec_lvl + 1);

    print_ast_list(node->body, buffer, rec_lvl + 1);

    break;
  }
  case ELSE_CONDITIONAL:
  {
    printf("@ ELSE_CONDITIONAL: \n");
    print_ast_list(node->body, buffer, rec_lvl + 1);

    break;
  }
  case ELSE_IF_CONDITIONAL:
  {
    printf("@ ELSE_IF_CONDITIONAL: \n");
    print_expression_tree(node->ast_data.exp, buffer, rec_lvl + 1);

    print_ast_list(node->body, buffer, rec_lvl + 1);

    break;
  }
  case WHILE_LOOP:
  {
    printf("@ WHILE_LOOP: \n");
    print_expression_tree(node->ast_data.exp, buffer, rec_lvl + 1);

    print_ast_list(node->body, buffer, rec_lvl + 1);

    break;
  }

  case FUNCTION_DECLARATION:
  {
    printf("@ FUNCTION DECLARATION: func %s\n", node->identifier.func_name);
    print_repeated_string(buffer, rec_lvl+1);
    printf("ACCESS MODIFIER: %s \n", access_modifer_to_string(node->access));
    print_repeated_string(buffer, rec_lvl+1);
    printf("FUNCTION ARGS:\n");
    for (int i = 0; i < node->ast_data.func_args.args_num; i++)
    {
      print_expression_tree(node->ast_data.func_args.func_prototype_args[i], buffer, rec_lvl + 1);
    }
    print_ast_list(node->body, buffer, rec_lvl + 1);

    break;
  }

  case OBJECT_DECLARATION: {
    printf("@ OBJECT DECLARATION: func %s\n", node->identifier.obj_name);
    print_repeated_string(buffer, rec_lvl+1);
    printf("ACCESS MODIFIER: %s \n", access_modifer_to_string(node->access));
    print_repeated_string(buffer, rec_lvl+1);
    printf("OBJECT ARGS:\n");
    for (int i = 0; i < node->ast_data.obj_args.args_num; i++)
    {
      print_expression_tree(node->ast_data.obj_args.object_prototype_args[i], buffer, rec_lvl + 1);
    }
    print_ast_list(node->body, buffer, rec_lvl + 1);

    break;
  }

  case INLINE_FUNCTION_DECLARATION:
  {
    printf("@ INLINE FUNCTION DECLARATION:\n");
    print_repeated_string(buffer, rec_lvl);
    printf("FUNCTION ARGS:\n");
    for (int i = 0; i < node->ast_data.func_args.args_num; i++)
    {
      print_expression_tree(node->ast_data.func_args.func_prototype_args[i], buffer, rec_lvl + 1);
    }
    print_ast_list(node->body, buffer, rec_lvl + 1);

    break;
  }

  case RETURN_VAL:
  {
    printf("@ RETURN VAL: \n");
    print_repeated_string(buffer, rec_lvl);
    printf("- RETURN EXPRESSION: \n");
    print_expression_tree(node->ast_data.exp, buffer, rec_lvl + 1);

    break;
  }
  case LOOP_TERMINATOR:
    printf("@ BREAK --\n");
    break;
  case LOOP_CONTINUATION:
    printf("@ CONTINUE --\n");
    break;

  case EXPRESSION_COMPONENT:
  {
    printf("@ EXPRESSION COMPONENT --\n");
    print_expression_component(node->identifier.expression_component, buffer, rec_lvl + 1);
    break;
  }

  default:
    printf("ast_node as invalid type: %d\n", node->type);
    return;
  }
  return;
}

/* Prints out a human readable representation of a abstract syntax tree list (struct ast_list) */
void print_ast_list(AST_List *list, char *buffer, int rec_lvl)
{
  if (!list)
  {
    printf("-- Code Block Empty \n");
    return;
  }

  AST_node *ptr = list->head;
  while (ptr)
  {
    print_ast_node(ptr, buffer, rec_lvl + 1);
    ptr = ptr->next;
  }

  return;
}
