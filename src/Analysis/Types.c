#include "ftc/Analysis/Types.h"

Type *createSimpleType(NodeKind K) {
  Type *Ty = (Type*) malloc(sizeof(Type));
  Ty->Kind = K;
  return Ty;
}

Type *createArrayType(NodeKind K) {
  Type *Ty = (Type*) malloc(sizeof(Type));
  Ty->Kind = ArrayTy;
  Ty->V.Ty.Kind = K;
  return Ty;
}

Type *createFunType(Type *From, Type To) {
  Type *Ty = (Type*) malloc(sizeof(Type));
  Ty->Kind = FunTy;
  Ty->V.Fn[0] = From;
  Ty->V.Fn[1] = To;
  return Ty;
}

Type *createSeqType(NodeKind K) {
  Type *Ty = (Type*) malloc(sizeof(Type));
  Ty->Kind = K;
  return Ty;
}
