#include "ftc/Analysis/Types.h"

#include <string.h>

Type *createType(NodeKind K, void *Ptr) {
  Type *Ty = (Type*) malloc(sizeof(Type));
  Ty->Kind = K;
  if (K < IntTy || K > StrConsumerTy) Ty->Val = Ptr;
  return Ty;
}

int compareType(Type *One, Type *Two) {
  if (!One && !Two) return 1;
  if (!One || !Two || One->Kind != Two->Kind) return 0;
  if (One->Kind >= IntTy && One->Kind <= StrConsumerTy) return 1;

  int I = 0, Ret = 1;
  Type **TyArr1, **TyArr2;
  PtrVector *V1, *V2;
  switch (One->Kind) {
    case IdTy:
      Ret = !strcmp(One->Val, Two->Val);
      break;
    case ArrayTy:
      Ret = compareType(One->Val, Two->Val);
      break;
    case RecordTy:
      Ret = One == Two;
      break;
    case FunTy:
      TyArr1 = (Type**) One->Val; 
      TyArr2 = (Type**) Two->Val; 
      Ret = compareType(TyArr1[0], TyArr2[0]) && 
        compareType(TyArr1[1], TyArr2[1]);
    case SeqTy:
      V1 = (PtrVector*) One->Val;
      V2 = (PtrVector*) Two->Val;
      if (V1->Size == V2->Size) 
        for (I = 0; I < V1->Size; ++I) 
          Ret = Ret && compareType(ptrVectorGet(V1, I), ptrVectorGet(V2, I)); 
      break;
  }
  return Ret;
}
