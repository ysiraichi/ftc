#include "ftc/Analysis/TypeCheck.h"
#include "ftc/Analysis/Types.h"
#include "ftc/Analysis/SymbolTable.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* <function> */
void semError(int Stop, ASTNode *Node, const char *S, ...) {
  printf("@[%u,%u]: ", Node->Pos[0], Node->Pos[1]);
  va_list Args;
  va_start(Args, S);
  vprintf(S, Args);
  va_end(Args);
  printf("\n");

  if (Stop) exit(1);
}

/* <function> */
Type *getRealType(SymbolTable *TyTable, Type *Ty) {
  if (!Ty) return NULL;
  switch (Ty->Kind) {
    case IdTy:
      {
        Type *Real = getRealType(TyTable, symTableFind(TyTable, Ty->Val));
        if (Real) return Real;
        return createType(IdTy, Ty->Val);
      }
    case ArrayTy:
    case RecordTy:
      return NULL;
    default:
      return createType(Ty->Kind, Ty->Val);
  }
}

/* <function> */
Type *resolveType(SymbolTable *TyTable, Type *Ty) {
  if (!Ty) return NULL;
  switch (Ty->Kind) {
    case IdTy:
      return getRealType(TyTable, Ty);
    case FunTy:
      {
        Type **Arr = (Type**) Ty->Val;
        Arr[0] = resolveType(TyTable, Arr[0]);
        Arr[1] = resolveType(TyTable, Arr[1]);
      } break;
    case SeqTy:
      {
        PtrVector *V = (PtrVector*) Ty->Val;
        PtrVectorIterator I = beginPtrVector(V),
                          E = endPtrVector(V);
        for (; I != E; ++I)
          *I = resolveType(TyTable, *I);
      } break;
    default:
      break;
  }
  return Ty;
}

/* <function> */
int typeEqual(SymbolTable *TyTable, Type *T1, Type *T2) {
  if (T1 == T2) return 1;
  T1 = resolveType(TyTable, T1);
  T2 = resolveType(TyTable, T2);
  if (T1->Kind == NilTy && T2->Kind == IdTy) T2 = symTableFind(TyTable, T2->Val);
  if (T2->Kind == NilTy && T1->Kind == IdTy) T1 = symTableFind(TyTable, T1->Val);
  return compareType(T1, T2);
}
