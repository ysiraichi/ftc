#include "ftc/Support/Hash.h"

#include <string.h>
#include <stdio.h>

static Pair *createPair(char *Key, void *Value) {
  Pair *P  = (Pair*) malloc(sizeof(Pair));
  P->Key   = Key;
  P->Value = Value;
  return P;
}

static unsigned hashHash(const char *S) { 
  unsigned N = 0;
  for (; *S; ++S) N = (N * 65599) + *S;
  return N % BUCK_SIZE;
}

static PtrVector *hashGetBucket(Hash *H, unsigned B) {
  return &(H->Pool[B]);
}

static Pair *hashGetPair(Hash *H, char *Key) {
  unsigned Bucket = hashHash(Key);
  PtrVector *V = hashGetBucket(H, Bucket);
  PtrVectorIterator I = beginPtrVector(V),
                    E = endPtrVector(V);
  for (; I != E; ++I) {
    Pair *P = (Pair*) *I;
    if (!strcmp(Key, P->Key)) return P;
  }
  return NULL;
}

static void hashChange(Hash *H, char *Key, void *New) {
  Pair *P = hashGetPair(H, Key);
  if (P) P->Value = New;
}

int hashInsert(Hash *H, char *Key, void *Value) {
  unsigned Bucket = hashHash(Key);
  if (!hashExists(H, Key)) {
    Pair *P = createPair(Key, Value);
    PtrVector *V = hashGetBucket(H, Bucket),
              *Pairs = &(H->Pairs);
    ptrVectorAppend(V, P);
    ptrVectorAppend(Pairs, P);
    return 1;
  } 
  return 0;
}

void hashInsertOrChange(Hash *H, char *Key, void *Value) {
  unsigned Bucket = hashHash(Key);
  if (hashExists(H, Key)) hashChange(H, Key, Value);
  else hashInsert(H, Key, Value);
}

void *hashFind(Hash *H, char *Key) {
  Pair *P = hashGetPair(H, Key);
  if (P) return P->Value;
  return NULL;
}

int hashExists(Hash *H, char *Key) {
  Pair *P = hashGetPair(H, Key);
  if (P) return 1;
  return 0;
}

void initHash(Hash *H) {
  int I;
  initPtrVector(&(H->Pairs), 0);
  for (I = 0; I < BUCK_SIZE; ++I) {
    initPtrVector(&(H->Pool[I]), 0);
  }
}

Hash *createHash(void) {
  Hash *H = (Hash*) malloc(sizeof(Hash));
  initHash(H);
  return H;
}

PtrVectorIterator beginHash(Hash *H) { return beginPtrVector(&(H->Pairs)); }
PtrVectorIterator endHash(Hash *H) { return endPtrVector(&(H->Pairs)); }
