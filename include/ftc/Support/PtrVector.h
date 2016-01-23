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
void destroyPtrVector(PtrVector*);
void destroyPtrVectorContents(PtrVector*);
int expandPtrVector(PtrVector*);
int appendToPtrVector(PtrVector*, void*);

PtrVector *createPtrVector(void);

void **beginPtrVector(PtrVector*);
void **endPtrVector(PtrVector*);

#endif
