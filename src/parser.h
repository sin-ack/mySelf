#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

enum ExprType {
  ENone,    // No expression (invalid)
  EMessage, // Message sending expression
  EIdent,   // Identifier expression
  ENumber,  // Number expression (integer or floating point)
  EBinary,  // Binary expression
  EObject,  // Object expression (activation record)
};

struct Expr {
  enum ExprType type;
  union {
    struct MessageExpr *message;
    struct IdentExpr *ident;
    struct NumberExpr *number;
    struct BinaryExpr *binary;
    struct ObjectExpr *object;
  };
};

struct MessageExpr {
  struct Expr receiver;
  const char *message;

  int length;
  struct Expr *args;
};

struct IdentExpr {
  const char *ident;
};

enum NumberType {
  NNone,    // Invalid
  NInteger, // Integer
  NFloat,   // Floating-point
};

struct NumberExpr {
  enum NumberType type;
  union {
    long integer;
    double flt;
  };
};

struct BinaryExpr {
  const char *op;
  struct Expr lhs, rhs;
};

enum AnnotationType { ACategory, AComment, AModule };

// Annotation carries metadata for a particular slot or object.
struct Annotation {
  const char *category;
  const char *comment;
  const char *module;
};

struct Slot {
  // Whether this slot will be searched in the message chain.
  bool parent;
  // If this slot is mutable, then it will receive messages to change its
  // value. For instance, if foo is mutable, it will receive foo: which will
  // replace the value it contains.
  bool mutable;

  // If this is non-zero, then it is the index in the arguments list this slot
  // takes when the method is called.
  int arg_index;

  const char *name;
  struct Expr value;

  struct Annotation annotation;
};

struct SlotList {
  struct Slot *slots;
  int length;
};

struct Stmt {
  // More stuff in the future perhaps?
  struct Expr expr;
};

struct StmtList {
  struct Stmt *stmts;
  int length;
};

struct ObjectExpr {
  struct SlotList slots;
  struct StmtList stmts;
  struct Annotation annotation;
};

typedef bool (*stmt_list_pred)(void);

struct Expr parse_expr(void);

struct StmtList parse_stmt_list(stmt_list_pred pred);
bool stmt_list_eof(void); // End of file
bool stmt_list_eoo(void); // End of object

#endif /* PARSER_H */
