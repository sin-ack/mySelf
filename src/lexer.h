#ifndef LEXER_H
#define LEXER_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

enum Tokens {
  TNull,         // null (invalid)
  TParenOpen,    // (
  TParenClose,   // )
  TBracketOpen,  // [
  TBracketClose, // ]
  TBraceOpen,    // {
  TBraceClose,   // }

  TPipe,   // |
  TColon,  // :
  TPeriod, // .

  TPlus,    // +
  TMinus,   // -
  TStar,    // *
  TSlash,   // /
  TPercent, // %
  TBang,    // !
  TTilde,   // ~
  TAmp,     // &
  TCap,     // ^
  TQuote,   // '
  TDQuote,  // "

  TEquals,    // =
  TLess,      // <
  TGreater,   // >
  TLeftArrow, // <-

  TIdent,   // identifier
  TInteger, // integer
  TFloat,   // float
  TString,  // string. uses ident in the union

  TEOF, // End of file
};

struct Token {
  enum Tokens type;
  union {
    const char *ident;
    long integer;
    double flt;
  };
};

#define TOKEN(t)                                                               \
  (lexer->offset++, lexer->column++, lexer->current = (struct Token){T##t, 0})
#define MAX_IDENT 64

const char *token_to_string(enum Tokens token);

struct Lexer {
  const char *filename;
  int line;
  int column;

  const char *data;
  long offset; // Actual offset into the file.
  long size;   // The size of the input.

  struct Token current; // The current token.
};

void lexer_init(struct Lexer *lexer, const char *fname);
struct Token lexer_lex(struct Lexer *lexer);

extern struct Lexer g_lexer;

struct Token lex();

#endif /* LEXER_H */
