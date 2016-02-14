#ifndef TYPES_H
#define TYPES_H

#include "ftc/Analysis/ParserTree.h"

typedef struct Type Type;

struct Type {
  NodeKind Kind;
  void    *Val;
  int      EscapedLevel;
};

Type *createType  (NodeKind, void*);
Type *createFnType(Type*, Type*);
Type *simplifyType(Type*);
void  destroyType (void*);
int   compareType (Type*, Type*);
void  checkIfNilTy(Type*, Type*);

void  dumpType(Type*);

#endif
