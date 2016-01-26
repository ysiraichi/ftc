#include "ftc/Support/Hash.h"

#include <string.h>

typedef struct {
  char *Key;
  void *Value;
} Pair;

static Pair *createPair(char *Key, void *Value) {
  Pair *P  = (Pair*) malloc(sizeof(Pair));
  P->Key   = Key;
  P->Value = Value;
  return P;
}

static unsigned hashHash(const char *S) { 
  unsigned N = 0;
  for (; *S; ++S) N = (N * 65599) + *S;
  return N;
}

static Pair *hashGetPair(Hash *H, char *Key) {
  unsigned Bucket = hashHash(Key);
  PtrVector *V = &((*H)[Bucket]);
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

void hashInsert(Hash *H, char *Key, void *Value) {
  unsigned Bucket = hashHash(Key);
  if (hashFind(H, Key)) 
    hashChange(H, Key, Value);
  else
    ptrVectorAppend(&((*H)[Bucket]), createPair(Key, Value));
}

void *hashFind(Hash *H, char *Key) {
  Pair *P = hashGetPair(H, Key);
  if (P) return P->Value;
  return NULL;
}


