#ifndef FAILURE_H
#define FAILURE_H

#include <stdarg.h>

void __attribute__((format(printf, 1, 2))) failure(const char *fmt, ...)
    __attribute__((noreturn));

#endif /* FAILURE_H */
