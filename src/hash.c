#include <stdio.h>
#include <stdlib.h>

#include "hash.h"

#define MAX_HASH_LENGTH 64
// Python: [(67**i) % 2**64 for i in range(1, 64)]
static uint64_t hash_multipliers[] = { //
    1ULL,
    67ULL,
    4489ULL,
    300763ULL,
    20151121ULL,
    1350125107ULL,
    90458382169ULL,
    6060711605323ULL,
    406067677556641ULL,
    27206534396294947ULL,
    1822837804551761449ULL,
    11449668462710707387ULL,
    10811279979525778673ULL,
    4932739753554658067ULL,
    16898914235099713017ULL,
    6975865255398123563ULL,
    6214370268935488321ULL,
    10534438397067581955ULL,
    4831097802565029577ULL,
    10088903518794604187ULL,
    11873749105694622353ULL,
    2331194912028978163ULL,
    8616106516265123993ULL,
    5430070304767207435ULL,
    13326573018921417441ULL,
    7436676729676490979ULL,
    195250898167001961ULL,
    13081810177189131387ULL,
    9484310407322876977ULL,
    8259498784508002515ULL,
    18430840424459171641ULL,
    17381199573934093291ULL,
    2395494809882498689ULL,
    12924199672450999235ULL,
    17371150663577574409ULL,
    1722217815995733595ULL,
    4708129229456841169ULL,
    1850009120545980851ULL,
    13270146634323407321ULL,
    3656108961609812939ULL,
    5151627469633295905ULL,
    13117647138658896547ULL,
    11885386825797142697ULL,
    3110922158897841211ULL,
    5517599835350293361ULL,
    744307494278622867ULL,
    12975113969248628857ULL,
    2335664475309207467ULL,
    8915567256040487361ULL,
    7047195796007001475ULL,
    10993516489730308425ULL,
    17142585937258151451ULL,
    4855125226303947025ULL,
    11698740909302073203ULL,
    9052389827437736729ULL,
    16214308079622709131ULL,
    16447485059567518049ULL,
    13623598642160163939ULL,
    8890649412962954729ULL,
    5377700309812315131ULL,
    9817783356943633073ULL,
    12155442335389109331ULL,
    2757897227850054073ULL,
    311673528858106731ULL};

uint64_t hash_string(void *_string) {
  const char *string = _string;
  const int m = (int)1e9 + 9;

  uint64_t sum = 0;
  int c;

  int length = strlen(string);
  length = length < MAX_HASH_LENGTH ? length : MAX_HASH_LENGTH;

  for (int i = 0; i < length; i++) {
    if (isupper(string[i])) {
      c = string[i] - 'A' + 1;
    } else if (islower(string[i])) {
      c = string[i] - 'a' + 27;
    } else if (isdigit(string[i])) {
      c = string[i] - '0' + 53;
    } else if (string[i] == ':') {
      c = 63;
    } else if (string[i] == '_') {
      c = 64;
    } else {
      fprintf(stderr, "internal error: hash got unexpected '%c'", string[i]);
      abort();
    }

    sum = (sum + c * hash_multipliers[i]) % m;
  }

  return sum;
}

// Hashtable implementation

void hashtable_init(struct HashTable *t, int capacity,
                    hashtable_hash_func hash) {
  t->buckets = calloc(capacity, sizeof(struct HashTableBucket *));
  t->capacity = capacity;
  t->bucket_size = 0;
  t->size = 0;

  t->hash = hash;
}

void hashtable_deinit(struct HashTable *t) {
  hashtable_clear(t);
  free(t->buckets);
}

bool hashtable_set(struct HashTable *t, void *key, void *value) {
  uint64_t hash = t->hash(key);
  int bucket = hash % t->capacity;

  // We "slot" the new bucket into the place.
  struct HashTableBucket **slot = t->buckets + bucket;
  if (*slot != NULL) {
    struct HashTableBucket *current = *slot;
    while (current->next != NULL) {
      if (current->entry.hash == hash) {
        printf("Collision epic\n");
        // Early exit: if the hash matches then we just set the value.
        current->entry.value = value;
        return true;
      }

      current = current->next;
    }
    slot = &current->next;
  } else {
    t->bucket_size++;
  }

  struct HashTableBucket *b = malloc(sizeof(*b));
  if (!b)
    return false;

  b->entry.hash = hash;
  b->entry.value = value;
  b->next = NULL;
  *slot = b;

  t->size++;

  return true;
}

bool hashtable_has(struct HashTable *t, void *key) {
  return hashtable_get(t, key) != NULL;
}

void *hashtable_get(struct HashTable *t, void *key) {
  uint64_t hash = t->hash(key);
  int bucket = hash % t->capacity;

  struct HashTableBucket *b = t->buckets[bucket];
  while (b != NULL) {
    if (b->entry.hash == hash)
      return b->entry.value;
    b = b->next;
  }

  return NULL;
}

void hashtable_remove(struct HashTable *t, void *key) {
  uint64_t hash = t->hash(key);
  int bucket = hash % t->capacity;

  struct HashTableBucket *b = t->buckets[bucket];
  struct HashTableBucket *previous = NULL;
  if (b == NULL)
    return;

  do {
    if (b->entry.hash == hash) {
      if (previous) {
        previous->next = b->next;
      } else {
        t->buckets[bucket] = b->next;
        if (b->next == NULL)
          t->bucket_size--;
      }

      t->size--;
      free(b);
      return;
    }

    previous = b;
    b = b->next;
  } while (b != NULL);
}

void hashtable_clear(struct HashTable *t) {
  for (int i = 0; i < t->capacity; i++) {
    if (t->buckets[i] != NULL) {
      struct HashTableBucket *b = t->buckets[i], *temp;
      while (b != NULL) {
        temp = b->next;
        free(b);
        b = temp;
      }
      t->buckets[i] = NULL;
    }
  }

  t->bucket_size = 0;
  t->size = 0;
}

struct HashTableIterator *hashtable_iterator_alloc(struct HashTable *t) {
  struct HashTableIterator *it = malloc(sizeof(*it));
  hashtable_iterator_init(it, t);

  return it;
}

void hashtable_iterator_init(struct HashTableIterator *it,
                             struct HashTable *t) {
  it->t = t;
  it->bucket_id = 0;
  it->bucket = NULL;
  it->end = false;

  it->bucket = t->buckets[0];
  if (it->bucket == NULL)
    hashtable_iterator_skip(it);
}

void hashtable_iterator_free(struct HashTableIterator *it) { free(it); }

struct HashTableEntry *hashtable_iterator_next(struct HashTableIterator *it) {
  if (it->end)
    return NULL;
  struct HashTableEntry *entry = &it->bucket->entry;
  hashtable_iterator_skip(it);
  return entry;
}

void hashtable_iterator_skip(struct HashTableIterator *it) {
  while (!it->end) {
    if (it->bucket == NULL) {
      it->bucket_id++;
      if (it->bucket_id >= it->t->capacity) {
        it->end = true;
        return;
      }
      it->bucket = it->t->buckets[it->bucket_id];
    } else {
      it->bucket = it->bucket->next;
    }

    if (it->bucket != NULL)
      return;
  }
}

#ifdef HASH_TEST
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct Integer {
  int i;
};

int main() {
  srand(time(NULL));

  puts("Initialization");
  struct HashTable t;
  hashtable_init(&t, HASHTABLE_DEFAULT_CAPACITY * 16, hash_string);

  puts("Insertion");
  char buf[20];
  for (int i = 0; i < 100000; i++) {
    sprintf(buf, "hash%06d", i);
    struct Integer *in = malloc(sizeof(*in));
    in->i = i;

    assert(hashtable_set(&t, buf, in) && "Failure during hashtable insert");
  }

  assert(t.size == 100000 && "Hashtable did not set all items");

  puts("Getting");
  for (int i = 0; i < 100000; i++) {
    sprintf(buf, "hash%06d", i);
    struct Integer *in = hashtable_get(&t, buf);
    assert(in && in->i == i && "Item missing from hashtable");
  }

  puts("Removal");
  for (int i = 0; i < 100000; i++) {
    sprintf(buf, "hash%06d", i);
    struct Integer *it = hashtable_get(&t, buf);
    hashtable_remove(&t, buf);
    free(it);
  }
  assert(t.size == 0 && "Hashtable removal failure");

  puts("Re-insertion");
  for (int i = 0; i < 100000; i++) {
    sprintf(buf, "hash%06d", i);
    struct Integer *in = malloc(sizeof(*in));
    in->i = i;

    assert(hashtable_set(&t, buf, in) && "Failure during hashtable insert");
  }

  puts("Iteration");
  struct HashTableIterator *it = hashtable_iterator_alloc(&t);
  int i = 0;
  while (!it->end) {
    struct HashTableEntry *entry = hashtable_iterator_next(it);

    i++;
    printf("Entry: %d\n", ((struct Integer *)entry->value)->i);
    free(entry->value);
  }
  assert(i == 100000 && "Iterator did not visit all nodes");
  hashtable_iterator_free(it);

  puts("Clearing");
  hashtable_clear(&t);
  assert(t.size == 0 && "Hashtable clear failure");

  hashtable_deinit(&t);
  return 0;
}
#endif
