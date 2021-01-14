#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "failure.h"
#include "lexer.h"

void failure(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  fprintf(stderr, "%s:%d:%d: ", g_lexer.filename, g_lexer.line, g_lexer.column);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);

  // Print the relevant line
  const char *line_start = g_lexer.data + g_lexer.offset - (g_lexer.column);
  long line_length = strchr(line_start, '\n') - line_start;
  fwrite(line_start, 1, line_length + 1, stderr);

  // Print the pointer
  for (int i = 0; i < g_lexer.column - 1; i++)
    fputc(' ', stderr);
  fputs("^\n", stderr);

  exit(1);
}
