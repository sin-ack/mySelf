#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

struct Stack {
  void **data;
  int length;
  int capacity;
};

void stack_init(struct Stack *s, int capacity);
void stack_deinit(struct Stack *s);

bool stack_push(struct Stack *s, void *value);
void *stack_top(struct Stack *s);
bool stack_pop(struct Stack *s);

bool stack_full(struct Stack *s);
bool stack_empty(struct Stack *s);

#endif /* STACK_H */
