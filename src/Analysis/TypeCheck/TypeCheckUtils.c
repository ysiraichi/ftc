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
char *getUnifiedId(SymbolTable *TyTable, Type *Ty) {
  if (!Ty) return NULL;
  switch (Ty->Kind) {
    case IdTy:
      {
        char *Real = getUnifiedId(TyTable, symTableFind(TyTable, Ty->Val));
        if (Real) return Real;
        return Ty->Val;
      }
    default: return NULL;
  }
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
      return NULL;
    default:
      return createType(Ty->Kind, Ty->Val);
  }
}

/* <function> */
Type *getRecordType(SymbolTable *TyTable, Type *Ty) {
  if (!Ty) return NULL;
  switch (Ty->Kind) {
    case IdTy:     return getRecordType(TyTable, symTableFind(TyTable, Ty->Val));
    case RecordTy: return Ty;

    default: return NULL;
  }
}

/* <function> */
Type *resolveType(SymbolTable *TyTable, Type *Ty) {
  if (!Ty) return NULL;
  switch (Ty->Kind) {
    case IdTy:
      {
        Type *RealType = getRealType(TyTable, Ty);
        if (RealType->Kind == RecordTy) 
          return createType(IdTy, getUnifiedId(TyTable, Ty));
        else return RealType;
      }
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
  if ((!T1 && T2) || (T1 && !T2)) return 0;
  T1 = simplifyType(resolveType(TyTable, T1));
  T2 = simplifyType(resolveType(TyTable, T2));

  switch (T1->Kind) {
    case IdTy:
      {
        if (T2->Kind == NilTy) {
          Type *RealType = getRealType(TyTable, T1);
          if (RealType->Kind == RecordTy) {
            T2->Val = T1->Val;
            return 1;
          }
        } else return !strcmp(T1->Val, T2->Val);
      }
    case FunTy:
      {
        if (T2->Kind != FunTy) return 0;
        Type **ArrTyOne = (Type**) T1->Val;
        Type **ArrTyTwo = (Type**) T2->Val;
        return typeEqual(TyTable, ArrTyOne[0], ArrTyTwo[0]) &&
          typeEqual(TyTable, ArrTyOne[1], ArrTyTwo[1]);
      }
    case SeqTy:
      {
        if (T2->Kind != SeqTy) return 0;
        int Success = 1;
        unsigned Count;
        PtrVector *V1 = (PtrVector*) T1->Val;
        PtrVector *V2 = (PtrVector*) T2->Val;
        if (V1->Size != V2->Size) return 0;
        for (Count = 0; Count < V1->Size; ++Count) {
          Success = Success && typeEqual(TyTable, ptrVectorGet(V1, Count), ptrVectorGet(V2, Count)); 
        }
        return Success;
      }
    case NilTy:
      {
        return T2->Kind != NilTy && typeEqual(TyTable, T2, T1);
      }
    default: return compareType(T1, T2);
  }
}
