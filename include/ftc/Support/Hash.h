#ifndef HASH_H
#define HASH_H

#include "ftc/Support/PtrVector.h"

#define BUCK_SIZE 10

typedef struct Hash Hash;
typedef struct Pair Pair;

struct Pair {
  void *first;
  void *second;
};

Pair *createPair(void*, void*);

struct Hash {
  PtrVector Pairs;
  PtrVector Pool[BUCK_SIZE];
};

int hashInsert(Hash*, char*, void*);
void hashInsertOrChange(Hash*, char*, void*);
void *hashFind(Hash*, const char*);
int hashExists(Hash*, const char*);

void initHash(Hash*);
Hash *createHash(void);
void destroyHash(Hash*, void (*)(void*));
void destroyHashContents(Hash*, void (*)(void*));

PtrVectorIterator beginHash(Hash*);
PtrVectorIterator endHash(Hash*);

#endif
