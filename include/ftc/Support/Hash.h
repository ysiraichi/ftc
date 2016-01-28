#ifndef HASH_H
#define HASH_H

#include "ftc/Support/PtrVector.h"

#define BUCK_SIZE 10

typedef struct Hash Hash;
typedef struct Pair Pair;

struct Pair {
  char *Key;
  void *Value;
};

struct Hash {
  PtrVector Pairs;
  PtrVector Pool[BUCK_SIZE];
};

int hashInsert(Hash*, char*, void*);
void hashInsertOrChange(Hash*, char*, void*);
void *hashFind(Hash*, char*);
int hashExists(Hash*, char*);

void initHash(Hash*);
Hash *createHash(void);
void destroyHash(Hash*, void (*)(void*));
void destroyHashContents(Hash*, void (*)(void*));

PtrVectorIterator beginHash(Hash*);
PtrVectorIterator endHash(Hash*);

#endif
