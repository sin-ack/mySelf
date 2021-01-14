#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "failure.h"
#include "lexer.h"
#include "parser.h"
#include "stack.h"

void _assert_token(int line, struct Token got, enum Tokens expected) {
  if (got.type != expected) {
    // TODO: recover from syntax error?
    failure("syntax error: expected %s, got %s (%d)", token_to_string(expected),
            token_to_string(got.type), line);
  }
}

#define assert_token(got, expected) _assert_token(__LINE__, got, expected)

struct IdentExpr *parse_ident_expr() {
  // Lexer pre-condition: standing on the ident.

  assert_token(g_lexer.current, TIdent);
  struct IdentExpr *ident = malloc(sizeof(*ident));
  ident->ident = strdup(g_lexer.current.ident);

  lex();
  return ident;
}

struct NumberExpr *parse_number_expr() {
  // Lexer pre-condition: standing on either an integer or float.
  if (g_lexer.current.type != TInteger && g_lexer.current.type != TFloat)
    assert_token(g_lexer.current, TInteger);

  struct NumberExpr *number = malloc(sizeof(*number));
  number->type = g_lexer.current.type == TInteger ? NInteger : NFloat;
  if (number->type == NInteger) {
    number->integer = g_lexer.current.integer;
  } else {
    number->flt = g_lexer.current.flt;
  }

  lex();
  return number;
}

struct Slot parse_slot(struct Stack *annotations) {
  struct Slot slot = {.parent = false, .mutable = false, .arg_index = 0};

  char *slot_name = NULL;
  int name_length = 0;
  char **param_names = NULL;
  int param_length = 0;

  // Get the slot name.
  assert_token(g_lexer.current, TIdent);

  while (g_lexer.current.type == TIdent) {
    int ident_len = strlen(g_lexer.current.ident);
    slot_name = realloc(slot_name, name_length + ident_len + 2);
    strcpy(slot_name + name_length, g_lexer.current.ident);
    name_length += ident_len;

    if (lex().type != TColon) {
      slot_name[name_length] = '\0';

      if (param_length != 0) {
        // We need either a unary message or a keyword one.
        // Mixing them is illegal.
        assert_token(g_lexer.current, TColon);
      } else {
        // Unary message
        slot.name = slot_name;
        break;
      }
    } else {
      // Keyword message. Look for an ident.
      slot_name[name_length++] = ':';
      slot_name[name_length] = '\0';

      assert_token(lex(), TIdent);
      param_names = realloc(param_names, (param_length + 1) * sizeof(char *));
      param_names[param_length++] = strdup(g_lexer.current.ident);
      lex();
    }
  }

  slot.name = slot_name;

  // Assign the slot attributes.
  if (g_lexer.current.type == TStar) {
    slot.parent = true;
    lex();
  }

  if (g_lexer.current.type == TLeftArrow) {
    slot.mutable = true;
  } else if (g_lexer.current.type != TEquals) {
    failure("syntax error: expected = or <- for slot assignment, got %s",
            token_to_string(g_lexer.current.type));
  }
  lex();

  slot.value = parse_expr();
  if (param_length > 0) {
    // Keyword slots require an object with code in it.
    if (slot.value.type != EObject)
      failure("syntax error: expected an object after keyword slot");

    // Inject the parameters.
    struct ObjectExpr *object = slot.value.object;

    if (!object->stmts.length)
      failure("syntax error: empty objects cannot have arguments");

    object->slots.slots =
        realloc(object->slots.slots,
                (object->slots.length + param_length) * sizeof(struct Slot));
    for (int i = 0; i < param_length; i++) {
      // This will cost us one IdentExpr alloc per new slot to initialize
      // them to nil. TODO: consider adding a NilExpr to avoid allocs?
      struct IdentExpr *ident = malloc(sizeof(struct IdentExpr));
      // TODO: owned parameter in identexpr to avoid duplicating constant
      // strings.
      ident->ident = strdup("nil");

      object->slots.slots[object->slots.length + i] =
          (struct Slot){.mutable = false,
                        .parent = false,
                        .arg_index = i + 1,
                        .name = param_names[i],
                        .value = (struct Expr){.type = EIdent, .ident = ident},
                        .annotation = {0}};
    }
    object->slots.length += param_length;

    free(param_names);
  }

  // Add slot annotations.
  if (!stack_empty(annotations)) {
    // We pick up the most recent comment and module, but concatenate
    // all categories. (Right now with \x7f, because that's what Self used,
    // but in the future I'd like to go with something more Unicode-friendly.)
    struct Annotation *top = stack_top(annotations);
    if (top->comment)
      slot.annotation.comment = strdup(top->comment);
    if (top->module)
      slot.annotation.module = strdup(top->module);

    char *category = NULL;
    int length = 0;

    for (int i = 0; i < annotations->length; i++) {
      struct Annotation *annot = annotations->data[i];
      if (!annot->category)
        continue;

      int cat_length = strlen(annot->category);
      category = realloc(category, length + cat_length + 2);

      strcpy(category + length, annot->category);
      length += cat_length;

      category[length++] = 0x7f;
      category[length] = 0;
    }

    if (category)
      category[--length] = 0;

    slot.annotation.category = category;
  }

  return slot;
}

enum AnnotationType parse_annotation(char const **annot_str) {
  enum AnnotationType type = AComment;

  if (g_lexer.current.type == TStar) {
    type = ACategory;
    lex();
  } else if (g_lexer.current.type == TLeftArrow) {
    type = AModule;
    lex();
  }

  if (g_lexer.current.type != TString) {
    failure("syntax error: expected *, <- or string for annotation");
  }

  *annot_str = strdup(g_lexer.current.ident);
  lex();
  return type;
}

struct SlotList parse_slot_list(struct Annotation *object_annot) {
  // Lexer pre-condition: standing on the opening |.

  assert_token(g_lexer.current, TPipe);
  lex();

  *object_annot = (struct Annotation){0};

  struct SlotList slots = {0};

  struct Stack annotation_stack;
  stack_init(&annotation_stack, 32);

  while (g_lexer.current.type != TPipe) {
    slots.slots =
        realloc(slots.slots, (slots.length + 1) * sizeof(struct Slot));

    if (g_lexer.current.type == TBraceOpen) {
      puts("Adding to the annotation stack");
      // Parse the annotation.
      if (lex().type == TBraceClose) {
        // We are annotating the object.
        assert_token(lex(), TEquals);
        lex();

        const char *annot_str = NULL;
        switch (parse_annotation(&annot_str)) {
        case ACategory:
          object_annot->category = annot_str;
          break;
        case AComment:
          object_annot->comment = annot_str;
          break;
        case AModule:
          object_annot->module = annot_str;
          break;
        }
        assert_token(g_lexer.current, TPeriod);
        lex();
      } else {
        // We need to see at least one annotation for this annotation block
        // to be valid.
        bool saw_annot = false;
        struct Annotation *annot = calloc(1, sizeof(struct Annotation));

        while (g_lexer.current.type == TString ||
               g_lexer.current.type == TStar ||
               g_lexer.current.type == TLeftArrow) {
          saw_annot = true;

          const char *annot_str = NULL;
          switch (parse_annotation(&annot_str)) {
          case ACategory:
            annot->category = annot_str;
            break;
          case AComment:
            annot->comment = annot_str;
            break;
          case AModule:
            annot->module = annot_str;
            break;
          }
          assert_token(g_lexer.current, TPeriod);
          lex();
        }

        if (!saw_annot) {
          failure("syntax error: annotation block with no annotations");
        }

        if (!stack_push(&annotation_stack, annot)) {
          failure("maximum annotation depth exceeded");
        }
      }

      // Reset back the loop.
    } else if (g_lexer.current.type == TBraceClose) {
      puts("Removing from the annotation stack");
      // Removing an annotation node.
      struct Annotation *annot = stack_top(&annotation_stack);
      if (!stack_pop(&annotation_stack))
        failure("internal error: unbalanced annotation stack");
      free(annot);

      lex();
    } else if (g_lexer.current.type == TIdent) {
      // The only thing we can expect at this point is that we have regular
      // slots, so anything else is illegal.

      slots.slots[slots.length++] = parse_slot(&annotation_stack);

      if (g_lexer.current.type == TPeriod) {
        lex();
      } else if (g_lexer.current.type != TPipe &&
                 g_lexer.current.type != TBraceClose) {
        failure("expected . or | after slot, got %s",
                token_to_string(g_lexer.current.type));
      }
    } else {
      failure("syntax error: expected annotation block or slot name");
    }
  }

  stack_deinit(&annotation_stack);

  lex();
  return slots;
}

// Keyword argument, tee hee
enum ObjectExprSubexpr { ObjectIsntSubexpr = false, ObjectIsSubexpr };
struct ObjectExpr *parse_object_expr(enum ObjectExprSubexpr is_subexpr) {
  // Lexer pre-condition: standing on the first parenthesis.

  assert_token(g_lexer.current, TParenOpen);

  // An object consists of (optional) slots, followed by code which is a list
  // of statements. The last statement/slot does not have to have a period
  // following it.

  struct ObjectExpr *expr = malloc(sizeof(*expr));
  expr->annotation = (struct Annotation){0};

  // Check for slots.
  if (lex().type == TPipe) {
    expr->slots = parse_slot_list(&expr->annotation);
  } else {
    expr->slots = (struct SlotList){.length = 0, .slots = NULL};
  }

  // TODO: disallow the usage of slot list delimiters inside sub-exprs.
  // Sub-exprs are treated by Self as anonymous objects that are immediately
  // executed. They are crippled a bit so it's not immediately obvious that
  // they're really objects, but you can make the ugly guts show by prepending
  // | | before the inner expression.
  if (expr->slots.length && is_subexpr && g_lexer.current.type != TParenClose) {
    failure("slots and code cannot be used together in sub-exprs");
  }

  if (g_lexer.current.type == TParenClose) {
    expr->stmts = (struct StmtList){.length = 0, .stmts = NULL};
    lex();
  } else {
    expr->stmts = parse_stmt_list(stmt_list_eoo);
    lex();

    if (is_subexpr && expr->stmts.length > 1) {
      failure("sub-expression cannot contain multiple expressions");
    }
  }

  return expr;
}

struct Expr parse_primary() {
  // Lexer pre-condition: standing on the first token of the primary expr.

  struct Expr primary;
  switch (g_lexer.current.type) {
  case TParenOpen: {
    struct ObjectExpr *expr = parse_object_expr(ObjectIsSubexpr);
    primary = (struct Expr){.type = EObject, .object = expr};
    break;
  }
  case TBracketOpen:
    failure("TODO support blocks");
  case TIdent: {
    struct IdentExpr *expr = parse_ident_expr();
    primary = (struct Expr){.type = EIdent, .ident = expr};
    break;
  }
  case TInteger:
  case TFloat: {
    struct NumberExpr *expr = parse_number_expr();
    primary = (struct Expr){.type = ENumber, .number = expr};
    break;
  }
  default:
    failure("syntax error: expected primary expression, got %s",
            token_to_string(g_lexer.current.type));
  }

  // Handle message expressions.
  //
  // Message expressions are listed as ident ident: expr OtherIdent: expr.
  // However we need to make sure to not take message keywords that start
  // with capital letters because they belong to the parent expression.
  // So self msg: 1 + 2 WithThree: 3 complement would be parsed as:
  // [self msg: [1 + 2] WithThree: [3 complement]]
  // (with each bracket representing a separate expression).
  //
  // Note that nested unary messages (as single idents next to each other)
  // are left-associative, while expressions after keywords are
  // right-associative.
  //
  // For example, self negate print becomes [[self negate] print], while
  // self add: 2 negate becomes [self add: [2 negate]].

  char *message_name = NULL;
  int length = 0;
  // TODO: better way than realloc'ing everytime.
  struct Expr *args = NULL;
  int argc = 0;

  if (g_lexer.current.type == TColon) {
    // This is a keyword message with an implicit receiver of "self".
    // Update it as such.

    struct IdentExpr *ident = malloc(sizeof(*ident));
    ident->ident = strdup("self");

    if (primary.type != EIdent)
      failure("expected identifier before self keyword message");

    int ident_len = strlen(primary.ident->ident);
    message_name = realloc(message_name, ident_len + 2);
    strcpy(message_name, primary.ident->ident);
    length = ident_len;
    message_name[length++] = ':';
    message_name[length] = '\0';

    // Clean up the identifier
    free((void *)primary.ident->ident);
    free(primary.ident);
    primary.ident = ident;

    lex();

    args = realloc(args, sizeof(struct Expr));
    args[0] = parse_expr();
    argc++;
  }
  // Check if this message is for us.
  else if (g_lexer.current.type != TIdent || isupper(g_lexer.current.ident[0]))
    // This message isn't for us, it's another keyword for the parent
    // expression. Or it's just not a message.
    return primary;

  // This message belongs to us, so keep parsing until we don't have
  // a legible message.
  //
  // FIXME: is this the right way to do it?
  while (g_lexer.current.type == TIdent) {
    char *msg_copy = strdup(g_lexer.current.ident);
    lex();

    // Standing on either a colon, an ident or something else.
    if (g_lexer.current.type == TColon) {
      // Keyword argument.

      int ident_len = strlen(msg_copy);
      message_name = realloc(message_name, length + ident_len + 2);
      strncpy(message_name + length, msg_copy, ident_len + 1);

      length += ident_len;
      message_name[length++] = ':';
      message_name[length] = '\0';

      args = realloc(args, (argc + 1) * sizeof(struct Expr));
      lex();
      args[argc++] = parse_expr();
    } else {
      // Unary message. Fold the current identifier into a message and
      // wait for the next iteration.

      struct MessageExpr *message = malloc(sizeof(*message));
      message->message = strdup(msg_copy);
      message->args = NULL;
      message->length = 0;
      message->receiver = primary;
      primary = (struct Expr){.type = EMessage, .message = message};
    }

    free(msg_copy);
  }

  if (argc > 0) {
    // We did actually have a keyword message.
    struct MessageExpr *message = malloc(sizeof(*message));
    message->message = message_name;
    message->length = argc;
    message->args = args;
    message->receiver = primary;

    return (struct Expr){.type = EMessage, .message = message};
  } else {
    // No keyword message, just return.
    return primary;
  }
}

struct Expr parse_expr() {
  // Lexer pre-condition: standing on the first token of the LHS expression.

  struct Expr lhs = parse_primary();

  // TODO: binary expressions.
  return lhs;
  // return parse_binary(lhs, 1);
}

struct Stmt parse_stmt() {
  // Lexer pre-condition: standing on the first token of the statement.

  struct Stmt stmt = {.expr = parse_expr()};

  if (g_lexer.current.type != TParenClose) {
    assert_token(g_lexer.current, TPeriod);
    lex();
  }

  return stmt;
}

struct StmtList parse_stmt_list(stmt_list_pred pred) {
  // Lexer pre-condition: standing on the first token of the first statement.

  // TODO: more efficient resizing of the statement list. Realloc after every
  // statement is slow.
  struct StmtList stmts = {0};

  while (!pred()) {
    stmts.stmts = realloc(stmts.stmts, ++stmts.length * sizeof(struct Stmt));
    stmts.stmts[stmts.length - 1] = parse_stmt();
  }

  return stmts;
}

bool stmt_list_eof() { return g_lexer.current.type == TEOF; }
bool stmt_list_eoo() { return g_lexer.current.type == TParenClose; }
