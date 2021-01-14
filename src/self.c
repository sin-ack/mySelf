#include <stdio.h>

#include "failure.h"
#include "lexer.h"
#include "object.h"
#include "parser.h"
#include "runtime.h"

int indent = 0;
#define PRINT_INDENT()                                                         \
  for (int iii = 0; iii < indent; iii++)                                       \
  putchar(' ')

void print_ast(struct StmtList *ast);

void print_expr(struct Expr expr) {
  printf("Expr {\n");
  indent += 2;
  PRINT_INDENT();
  printf("type = ");

  switch (expr.type) {
  case EIdent:
    printf("EIdent,\n");
    PRINT_INDENT();
    printf("ident = IdentExpr {\n");
    indent += 2;

    PRINT_INDENT();
    printf("ident = \"%s\"\n", expr.ident->ident);

    indent -= 2;
    PRINT_INDENT();
    printf("}\n");
    break;
  case EMessage:
    printf("EMessage,\n");
    PRINT_INDENT();
    printf("message = MessageExpr {\n");
    indent += 2;

    PRINT_INDENT();
    printf("receiver = ");
    print_expr(expr.message->receiver);
    printf(",\n");
    PRINT_INDENT();
    printf("message = \"%s\",\n", expr.message->message);
    PRINT_INDENT();
    printf("length = %d,\n", expr.message->length);
    PRINT_INDENT();
    printf("args = [\n");

    indent += 2;
    for (int i = 0; i < expr.message->length; i++) {
      PRINT_INDENT();
      print_expr(expr.message->args[i]);
      printf(",\n");
    }
    indent -= 2;
    PRINT_INDENT();
    printf("]\n");

    indent -= 2;
    PRINT_INDENT();
    printf("}\n");
    break;
  case ENumber:
    printf("ENumber,\n");
    PRINT_INDENT();
    printf("number = NumberExpr {\n");
    indent += 2;

    PRINT_INDENT();
    printf("type = %s,\n",
           expr.number->type == NInteger ? "NInteger" : "NFloat");
    PRINT_INDENT();
    if (expr.number->type == NInteger) {
      printf("integer = %ld\n", expr.number->integer);
    } else {
      printf("flt = %lf\n", expr.number->flt);
    }

    indent -= 2;
    PRINT_INDENT();
    printf("}\n");
    break;
  case EObject:
    printf("EObject,\n");
    PRINT_INDENT();
    printf("object = ObjectExpr {\n");
    indent += 2;

    PRINT_INDENT();
    printf("slots = SlotList {\n");
    indent += 2;

    PRINT_INDENT();
    printf("length = %d,\n", expr.object->slots.length);
    PRINT_INDENT();
    printf("slots = [\n");
    indent += 2;

    for (int i = 0; i < expr.object->slots.length; i++) {
      PRINT_INDENT();
      printf("Slot {\n");
      indent += 2;

      PRINT_INDENT();
      printf("parent = %d,\n", expr.object->slots.slots[i].parent);
      PRINT_INDENT();
      printf("mutable = %d,\n", expr.object->slots.slots[i].mutable);
      PRINT_INDENT();
      printf("arg_index = %d,\n", expr.object->slots.slots[i].arg_index);
      PRINT_INDENT();
      printf("name = \"%s\",\n", expr.object->slots.slots[i].name);
      PRINT_INDENT();
      printf("annotations = Annotation {\n");
      indent += 2;

      PRINT_INDENT();
      printf("module = %s,\n", expr.object->slots.slots[i].annotation.module);
      PRINT_INDENT();
      printf("category = %s,\n",
             expr.object->slots.slots[i].annotation.category);
      PRINT_INDENT();
      printf("comment = %s\n", expr.object->slots.slots[i].annotation.comment);

      indent -= 2;
      PRINT_INDENT();
      printf("},\n");
      PRINT_INDENT();
      printf("value = ");
      print_expr(expr.object->slots.slots[i].value);
      putchar('\n');

      indent -= 2;
      PRINT_INDENT();
      printf("},\n");
    }

    indent -= 2;
    PRINT_INDENT();
    printf("]\n");
    indent -= 2;
    PRINT_INDENT();
    printf("},\n");
    PRINT_INDENT();
    printf("annotations = Annotation {\n");
    indent += 2;

    PRINT_INDENT();
    printf("module = %s,\n", expr.object->annotation.module);
    PRINT_INDENT();
    printf("category = %s,\n", expr.object->annotation.category);
    PRINT_INDENT();
    printf("comment = %s\n", expr.object->annotation.comment);

    indent -= 2;
    PRINT_INDENT();
    printf("},\n");
    PRINT_INDENT();
    printf("stmts = ");
    print_ast(&expr.object->stmts);

    indent -= 2;
    PRINT_INDENT();
    printf("}\n");
    break;
  case ENone:
    failure("ENone reached");
  case EBinary:
    failure("TODO EBinary");
  }

  indent -= 2;
  PRINT_INDENT();
  printf("}");
}

void print_ast(struct StmtList *ast) {
  printf("StmtList {\n");
  indent += 2;
  PRINT_INDENT();
  printf("length = %d,\n", ast->length);
  PRINT_INDENT();
  printf("stmts = [\n");
  indent += 2;

  for (int i = 0; i < ast->length; i++) {
    PRINT_INDENT();
    printf("Stmt {\n");
    indent += 2;

    PRINT_INDENT();
    print_expr(ast->stmts[i].expr);
    putchar('\n');

    indent -= 2;
    PRINT_INDENT();
    printf("}%s\n", i == ast->length - 1 ? "" : ",");
  }

  indent -= 2;
  PRINT_INDENT();
  printf("]\n");
  indent -= 2;
  PRINT_INDENT();
  printf("}\n");
}

int main(int argc, char **argv) {
  puts("mySelf, v0.10");

  if (argc != 2) {
    puts("Usage: ./mySelf [world script]");
    return 1;
  }

  const char *fname = argv[1];
  lexer_init(&g_lexer, fname);

  struct StmtList ast = parse_stmt_list(stmt_list_eof);
  print_ast(&ast);

  // The root object which will be populated by the world script.
  struct Object *lobby = object_create();
  // An initial NIL value. This is necessary because objects without code will
  // implicitly return a nil if they get activated.
  struct Object *nil = object_create();

  for (int i = 0; i < ast.length; i++) {
    execute(&ast.stmts[i], lobby);
  }

  return 0;
}
