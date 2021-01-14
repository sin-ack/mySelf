#include <stdlib.h>
#include <string.h>

#include "failure.h"
#include "lexer.h"

struct Lexer g_lexer;

const char *token_to_string(enum Tokens token) {
  switch (token) {
  case TNull:
    failure("internal error: reached null token");
  case TParenOpen:
    return "(";
  case TParenClose:
    return ")";
  case TBracketOpen:
    return "[";
  case TBracketClose:
    return "]";
  case TBraceOpen:
    return "{";
  case TBraceClose:
    return "}";
  case TPipe:
    return "|";
  case TColon:
    return ":";
  case TPeriod:
    return ".";
  case TPlus:
    return "+";
  case TMinus:
    return "-";
  case TStar:
    return "*";
  case TSlash:
    return "/";
  case TPercent:
    return "%";
  case TBang:
    return "!";
  case TTilde:
    return "~";
  case TAmp:
    return "&";
  case TCap:
    return "^";
  case TQuote:
    return "'";
  case TDQuote:
    return "\"";
  case TEquals:
    return "=";
  case TLess:
    return "<";
  case TGreater:
    return ">";
  case TLeftArrow:
    return "<-";
  case TIdent:
    return "identifier";
  case TInteger:
    return "number";
  case TFloat:
    return "number"; // Both of them will look like numbers to the parser.
  case TString:
    return "string";
  case TEOF:
    return "end of file";
  default:
    __builtin_unreachable();
  }
}

void lexer_init(struct Lexer *lexer, const char *fname) {
  FILE *f = fopen(fname, "r");

  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *data = malloc(length);
  fread(data, 1, length, f);
  fclose(f);

  lexer->filename = fname;
  lexer->line = 1;
  lexer->column = 0;
  lexer->size = length;
  lexer->offset = 0;
  lexer->data = data;
  lexer->current = (struct Token){0};

  lexer_lex(lexer);
}

struct Token lexer_lex(struct Lexer *lexer) {
#define CUR lexer->data[lexer->offset]
#define CAN_PEEK (lexer->offset < lexer->size)
#define PEEK lexer->data[lexer->offset + 1]
#define EOF_CHECK(body)                                                        \
  do {                                                                         \
    if (lexer->offset == lexer->size) {                                        \
      body;                                                                    \
    }                                                                          \
  } while (0)

  if (lexer->current.type == TEOF)
    failure("tried to lex after EOF");

  // Identifier buffer.
  char buf[MAX_IDENT + 1];
  int length = 0;

  // Clear the previous ident if there was any.
  if (lexer->current.type == TIdent)
    free((void *)lexer->current.ident);

  // Skip whitespace.
  while (1) {
    if (CUR == ' ') {
      lexer->offset++;
      lexer->column++;
    } else if (CUR == '\t') {
      lexer->offset++;
      lexer->column++;
    } else if (CUR == '\n') {
      lexer->offset++;
      lexer->line++;
      lexer->column = 0;
    } else if (CUR == '"') {
      // Comments are syntactically important, but for now let's skip them.
      lexer->offset++;
      while (CUR != '"') {
        if (CUR == '\n') {
          lexer->line++;
          lexer->column = 0;
        } else {
          lexer->column++;
        }

        lexer->offset++;
        EOF_CHECK(failure("EOF while scanning comment"));
      }
      lexer->offset++;
      lexer->column++;
    } else {
      break;
    }

    EOF_CHECK(return TOKEN(EOF));
  }

  // Identifier.
  if (CUR == '_' || (CUR >= 'a' && CUR <= 'z') || (CUR >= 'A' && CUR <= 'Z')) {
    goto ident_loop;
    while (CUR == '_' || (CUR >= 'a' && CUR <= 'z') ||
           (CUR >= 'A' && CUR <= 'Z') || (CUR >= '0' && CUR <= '9')) {
      if (length == MAX_IDENT) {
        failure("maximum identifier length reached");
      }

    ident_loop:
      buf[length++] = CUR;
      lexer->offset++;
      lexer->column++;

      EOF_CHECK(goto ident_out);
    }
  ident_out:;

    const char *copy = strndup(buf, length);
    return lexer->current = (struct Token){TIdent, .ident = copy};
  }

  // Number (integer or float).
  if ((CUR >= '0' && CUR <= '9')) {
    enum Tokens type = TInteger;

    goto number_loop;
    while ((CUR >= '0' && CUR <= '9') || CUR == '.') {
      if (length == MAX_IDENT) {
        failure("maximum number length reached");
      }

      if (CUR == '.') {
        // If the next character after the . is a digit, then we consider it
        // a floating point number. Otherwise, the . is the statement
        // terminator.

        // Note that a . after a float is just a regular statement terminator.
        if (type == TFloat) {
          goto number_out;
        }

        if (CAN_PEEK && (PEEK >= '0' && PEEK <= '9')) {
          type = TFloat;
        } else {
          goto number_out;
        }
      }
    number_loop:
      buf[length++] = CUR;
      lexer->offset++;
      lexer->column++;

      EOF_CHECK(goto number_out);
    }

  number_out:;

    buf[length] = 0;
    if (type == TInteger) {
      return lexer->current =
                 (struct Token){TInteger, .integer = strtol(buf, NULL, 10)};
    } else {
      return lexer->current = (struct Token){TFloat, .flt = strtod(buf, NULL)};
    }
  }

  // Strings.
  if (CUR == '\'') {
    // TODO: support strings longer than 64 characters.
    lexer->offset++;
    lexer->column++;
    do {
      EOF_CHECK(failure("EOF while scanning string literal"));

      if (length == MAX_IDENT)
        failure("maximum string length reached");

      if (CUR == '\n')
        failure("unexpected newline in string literal");

      buf[length++] = CUR;
      lexer->offset++;
      lexer->column++;
    } while (CUR != '\'');

    lexer->offset++;
    lexer->column++;
    buf[length] = '\0';

    struct Token t = {.type = TString, .ident = strdup(buf)};
    return lexer->current = t;
  }

  // The rest is regular tokens.
  switch (CUR) {
  case '(':
    return TOKEN(ParenOpen);
  case ')':
    return TOKEN(ParenClose);
  case '[':
    return TOKEN(BracketOpen);
  case ']':
    return TOKEN(BracketClose);
  case '{':
    return TOKEN(BraceOpen);
  case '}':
    return TOKEN(BraceClose);
  case '|':
    return TOKEN(Pipe);
  case ':':
    return TOKEN(Colon);
  case '.':
    return TOKEN(Period);
  case '+':
    return TOKEN(Plus);
  case '-':
    return TOKEN(Minus);
  case '*':
    return TOKEN(Star);
  case '/':
    return TOKEN(Slash);
  case '%':
    return TOKEN(Percent);
  case '!':
    return TOKEN(Bang);
  case '~':
    return TOKEN(Tilde);
  case '&':
    return TOKEN(Amp);
  case '^':
    return TOKEN(Cap);
  case '=':
    return TOKEN(Equals);
  case '<': {
    if (CAN_PEEK && PEEK == '-') {
      lexer->offset++;
      lexer->column++;
      return TOKEN(LeftArrow);
    }
    return TOKEN(Less);
  }
  case '>':
    return TOKEN(Greater);
  default:
    failure("syntax error: unknown character '%c'", CUR);
  }

  __builtin_unreachable();

#undef CUR
#undef PEEK
#undef CAN_PEEK
#undef EOF_CHECK
}

struct Token lex() {
  return lexer_lex(&g_lexer);
}
