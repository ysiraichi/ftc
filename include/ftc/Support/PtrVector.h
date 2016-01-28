#ifndef PTR_VECTOR_H
#define PTR_VECTOR_H

#include <stdlib.h>

#define STD_SIZE 2

typedef void** PtrVectorIterator;
typedef struct PtrVector PtrVector;

struct PtrVector {
  size_t MaxSize, Size;
  void **Head;
};

void initPtrVector(PtrVector*, size_t);
void destroyPtrVector(PtrVector*, void (*)(void*));
void destroyPtrVectorContents(PtrVector*, void (*)(void*));
int expandPtrVector(PtrVector*);
int ptrVectorAppend(PtrVector*, void*);
void *ptrVectorGet(PtrVector*, unsigned);

PtrVector *createPtrVector(void);

void **beginPtrVector(PtrVector*);
void **endPtrVector(PtrVector*);

#endif
