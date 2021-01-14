#ifndef HASH_H
#define HASH_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Hash functions
uint64_t hash_string(void *string);

// A hashmap implementation. This is a simple singly-linked list hashmap with
// buckets. The user selects the capacity of the hashmap, which is then
// initialized with these buckets. Buckets are lazily initialized when a new
// object is to be inserted.

struct HashTableEntry {
  uint64_t hash;
  // Note that primitives such as integers cannot be directly added here.
  // Instead, they must be added by either writing a structure that holds
  // them (boxing), or by heap-allocating a single integer.
  void *value;
};

struct HashTableBucket {
  struct HashTableEntry entry;
  struct HashTableBucket *next;
};

typedef uint64_t (*hashtable_hash_func)(void *);

struct HashTable {
  struct HashTableBucket **buckets;
  int capacity;
  int bucket_size;
  int size;

  hashtable_hash_func hash;
};

#define HASHTABLE_DEFAULT_CAPACITY 256
void hashtable_init(struct HashTable *t, int capacity,
                    hashtable_hash_func hash);
void hashtable_deinit(struct HashTable *t);

bool hashtable_set(struct HashTable *t, void *key, void *value);
bool hashtable_has(struct HashTable *t, void *key);
void *hashtable_get(struct HashTable *t, void *key);
void hashtable_remove(struct HashTable *t, void *key);
void hashtable_clear(struct HashTable *t);

struct HashTableIterator {
  struct HashTable *t;

  int bucket_id;
  struct HashTableBucket *bucket;
  bool end;
};

struct HashTableIterator *hashtable_iterator_alloc(struct HashTable *t);
void hashtable_iterator_init(struct HashTableIterator *it, struct HashTable *t);
void hashtable_iterator_free(struct HashTableIterator *it);

struct HashTableEntry *hashtable_iterator_next(struct HashTableIterator *it);
void hashtable_iterator_skip(struct HashTableIterator *it);

#endif /* HASH_H */
