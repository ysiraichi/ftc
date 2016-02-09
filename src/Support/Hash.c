#include "ftc/Support/Hash.h"

#include <string.h>
#include <stdio.h>

static void destroyPair(Pair *P, void (*destroyValue)(void*)) {
  if (destroyValue) (*destroyValue)(P->second);
  free(P);
}

Pair *createPair(void *F, void *S) {
  Pair *P  = (Pair*) malloc(sizeof(Pair));
  P->first   = F;
  P->second  = S;
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

static Pair *hashGetPair(Hash *H, const char *Key) {
  unsigned Bucket = hashHash(Key);
  PtrVector *V = hashGetBucket(H, Bucket);
  PtrVectorIterator I = beginPtrVector(V),
                    E = endPtrVector(V);
  for (; I != E; ++I) {
    Pair *P = (Pair*) *I;
    if (!strcmp(Key, P->first)) return P;
  }
  return NULL;
}

static void hashChange(Hash *H, const char *Key, void *New) {
  Pair *P = hashGetPair(H, Key);
  if (P) P->second = New;
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
  if (hashExists(H, Key)) hashChange(H, Key, Value);
  else hashInsert(H, Key, Value);
}

void *hashFind(Hash *H, const char *Key) {
  Pair *P = hashGetPair(H, Key);
  if (P) return P->second;
  return NULL;
}

int hashExists(Hash *H, const char *Key) {
  Pair *P = hashGetPair(H, Key);
  if (P) return 1;
  return 0;
}

void initHash(Hash *H) {
  int I;
  initPtrVector(&(H->Pairs), 0);
  for (I = 0; I < BUCK_SIZE; ++I) 
    initPtrVector(&(H->Pool[I]), 0);
}

Hash *createHash(void) {
  Hash *H = (Hash*) malloc(sizeof(Hash));
  initHash(H);
  return H;
}

void destroyHashContents(Hash *H, void (*destroyElem)(void*)) {
  PtrVectorIterator I = beginPtrVector(&(H->Pairs)),
                    E = endPtrVector(&(H->Pairs));
  for (; I != E; ++I) 
    destroyPair(*I, destroyElem);

  destroyPtrVectorContents(&(H->Pairs), NULL);
  int B = 0;
  for (B = 0; B < BUCK_SIZE; ++B) 
    destroyPtrVectorContents(hashGetBucket(H, B), NULL);
}

void destroyHash(Hash *H, void (*destroyElem)(void*)) {
  destroyHashContents(H, destroyElem);
  free(H);
}

PtrVectorIterator beginHash(Hash *H) { return beginPtrVector(&(H->Pairs)); }
PtrVectorIterator endHash(Hash *H) { return endPtrVector(&(H->Pairs)); }
