#include "ftc/Support/PtrVector.h"

#include <stdlib.h>
#include <stdio.h>

/* function */
void initPtrVector(PtrVector *V, size_t MaxSize) {
  if (!MaxSize) MaxSize = STD_SIZE;
  V->Head = malloc(sizeof(void*) * MaxSize);
  V->MaxSize = MaxSize;
  V->Size  = 0;
}

/* function */
void destroyPtrVectorContents(PtrVector *V) {
  free(V->Head);
}

/* function */
void destroyPtrVector(PtrVector *V) {
  destroyPtrVectorContents(V);
  free(V);
}

/* function */
int expandPtrVector(PtrVector *V) {
  size_t NewSize = 2 * V->MaxSize;
  void **NewHead = realloc(V->Head, NewSize * sizeof(void*));
  if (!NewHead) return 0;
  V->Head    = NewHead;
  V->MaxSize = NewSize;
  return 1;
}

/* function */
int appendToPtrVector(PtrVector *V, void *Elem) {
  if (V->Size == V->MaxSize && !expandPtrVector(V)) return 0;
  V->Head[V->Size++] = Elem;
  return 1;
}

/* function */
PtrVector *createPtrVector(void) {
  PtrVector *V = (PtrVector*) malloc(sizeof(PtrVector));
  initPtrVector(V, STD_SIZE);
  return V;
}

/* function */
void **beginPtrVector(PtrVector *V) { return V->Head; }
/* function */
void **endPtrVector(PtrVector *V) { return &(V->Head[V->Size]); }
