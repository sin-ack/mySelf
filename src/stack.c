#include <stdlib.h>

#include "stack.h"

void stack_init(struct Stack *s, int capacity) {
  s->data = calloc(capacity, sizeof(void *));
  s->length = 0;
  s->capacity = capacity;
}

void stack_deinit(struct Stack *s) { free(s->data); }

bool stack_push(struct Stack *s, void *value) {
  if (stack_full(s))
    return false;

  s->data[s->length++] = value;
  return true;
}

void *stack_top(struct Stack *s) { return s->data[s->length - 1]; }

bool stack_pop(struct Stack *s) {
  if (stack_empty(s))
    return false;

  s->length--;
  return true;
}

bool stack_full(struct Stack *s) { return s->length == s->capacity; }

bool stack_empty(struct Stack *s) { return s->length == 0; }
