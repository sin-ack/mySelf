#ifndef OBJECT_H
#define OBJECT_H

#include "hash.h"

// A slot within an object. Held in a slot map
struct ObjectSlot {
  // The exact name of the slot. Keyword messages look like foo:WithBar:.
  const char *name;
  // Whether this slot has an implicit slot accepting a value in order to change
  // the current value.
  bool mutable;
  // Whether the object in slot will be searched for slots.
  bool parent;
  // The value of this slot.
  struct Object *value;
};

struct Object {
  struct HashMap *slot_map;
};

struct Object *create_object(void);

#endif /* OBJECT_H */
