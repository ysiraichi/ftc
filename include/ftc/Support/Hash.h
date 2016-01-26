#ifndef HASH_H
#define HASH_H

#include "ftc/Support/PtrVector.h"

#define BUCK_SIZE 100

typedef PtrVector Hash[BUCK_SIZE];

void hashInsert(Hash*, char*, void*);
void *hashFind(Hash*, char*);

#endif
