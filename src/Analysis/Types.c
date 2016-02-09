#include "ftc/Analysis/Types.h"
#include "ftc/Support/PtrVector.h"
#include "ftc/Support/Hash.h"

#include <string.h>
#include <stdio.h>

/* <function> */
static Type *simplifyType(Type *Ty) {
  if (!Ty) return NULL;
  if (Ty->Kind != SeqTy) return Ty;

  PtrVector *V = (PtrVector*) Ty->Val;
  if (V->Size == 0) return Ty;
  else if (V->Size == 1) {
    Type *TheTy = ptrVectorGet(V, 0);
    return createType(TheTy->Kind, TheTy->Val);
  }
  return Ty;
}

/* <function> */
void dumpType(Type *Ty) {
  if (!Ty) return;
  else {
    switch (Ty->Kind) {
      case IntTy:
        printf("int");
        break;
      case FloatTy:
        printf("float");
        break;
      case StringTy:
        printf("string");
        break;
      case AnswerTy:
        printf("answer");
        break;
      case IdTy:
        printf("%s", (char*) Ty->Val);
        break;
      case ArrayTy:
        printf("array of ");
        dumpType(Ty->Val);
        break;
      case FunTy:
        {
          Type **Arr = (Type**) Ty->Val;
          dumpType(Arr[0]);
          printf(" -> ");
          dumpType(Arr[1]);
        } break;
      case RecordTy:
        {
          Hash *H = (Hash*) Ty->Val;
          PtrVectorIterator I = beginHash(H),
                            E = endHash(H);
          printf("{ ");
          for (; I != E; ++I) {
            Pair *P = (Pair*) *I;
            printf("%s:", (char*)P->first);
            dumpType(P->second);
            if (I+1 != E) printf(", ");
          }
          printf(" }");
        } break;
      case SeqTy:
        {
          PtrVector *V = (PtrVector*) Ty->Val;
          PtrVectorIterator I = beginPtrVector(V),
                            E = endPtrVector(V);
          printf("( ");
          for (; I != E; ++I) {
            dumpType(*I);
            if (I+1 != E) printf(", ");
          }
          printf(" )");
        } break;
      case NilTy:
        printf("nil");
        break;
      default:
        break;
    }
  }
}

/* <function> */
Type *createType(NodeKind K, void *Ptr) {
  Type *Ty = (Type*) malloc(sizeof(Type));
  Ty->Kind = K;
  if (K < IntTy || K > StrConsumerTy) Ty->Val = Ptr;
  return Ty;
}

/* <function> */
Type *createFnType(Type *From, Type *To) {
  Type **TyArr = (Type**) malloc(sizeof(Type*) * 2);
  TyArr[0] = From;
  TyArr[1] = To;
  return createType(FunTy, TyArr);
}

/* <function> */
void destroyType(void *T) {
  if (!T) return;
  Type *Ty = (Type*) T;
  switch (Ty->Kind) {
    case ArrayTy:
      destroyType(Ty->Val);
      break;
    case FunTy:
      {
        Type **Arr = (Type**) Ty->Val;
        destroyType(Arr[0]);
        destroyType(Arr[1]);
      } break;
    case RecordTy:
      {
        Hash *H = (Hash*) Ty->Val;
        destroyHash(H, &destroyType);
      } break;
    case SeqTy:
      {
        PtrVector *V = (PtrVector*) Ty->Val;
        destroyPtrVector(V, &destroyType);
      } break;
    default:
      break;
  }
  free(Ty);
}

/* <function> */
int compareType(Type *One, Type *Two) {
  One = simplifyType(One);
  Two = simplifyType(Two);

  /*
     printf("------------------\n");
     dumpType(One);
     printf(" == ");
     dumpType(Two);
     printf("\n");
     printf("------------------\n");
     */

  if (!One && !Two) return 1;
  if (!One || !Two) return 0;

  if (One->Kind != Two->Kind && 
      ((One->Kind == NilTy && Two->Kind != RecordTy) || 
       (One->Kind != RecordTy && Two->Kind == NilTy) || 
       (One->Kind != NilTy && Two->Kind != NilTy))) 
    return 0;

  if (One->Kind >= IntTy && One->Kind <= StrConsumerTy) return 1;

  switch (One->Kind) {
    case IdTy:
      return !strcmp(One->Val, Two->Val);
    case ArrayTy:
      return compareType(One->Val, Two->Val);
    case RecordTy:
      return (One == Two) || (Two->Kind == NilTy);
    case FunTy:
      {
        Type **TyArr1 = (Type**) One->Val,
             **TyArr2 = (Type**) Two->Val; 
        return compareType(TyArr1[0], TyArr2[0]) && 
          compareType(TyArr1[1], TyArr2[1]);
      }
    case SeqTy:
      {
        unsigned I = 0, Ret = 1;
        PtrVector *V1 = (PtrVector*) One->Val,
                  *V2 = (PtrVector*) Two->Val;
        if (V1->Size == V2->Size) 
          for (I = 0; I < V1->Size; ++I) 
            Ret = Ret && compareType(ptrVectorGet(V1, I), ptrVectorGet(V2, I)); 
        else Ret = 0;
        return Ret;
      }
    case NilTy:
      return Two->Kind == RecordTy || Two->Kind == NilTy;
    default:
      break;
  }
  return 0;
}
