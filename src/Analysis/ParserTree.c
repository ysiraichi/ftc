#include "ftc/Analysis/ParserTree.h"

#include <stdlib.h>

NilT *createNil(void) {
  return (NilT*) malloc(sizeof(NilT));
}

ExprT *createExpr(ExprKind K, void *Ptr) {
  ExprT *Expr = (ExprT*) malloc(sizeof(ExprT));
  Expr->Kind = K;
  switch (K) {
    case Lit:     Expr->Value.Lit     = (LitT*)     Ptr; break;
    case Lval:    Expr->Value.Lval    = (LvalT*)    Ptr; break;
    case BinOp:   Expr->Value.BinOp   = (BinOpT*)   Ptr; break;
    case Neg:     Expr->Value.Neg     = (NegT*)     Ptr; break;
    case Let:     Expr->Value.Let     = (LetT*)     Ptr; break;
    case IfStmt:  Expr->Value.IfStmt  = (IfStmtT*)  Ptr; break;
    case FunCall: Expr->Value.FunCall = (FunCallT*) Ptr; break;
    case Create:  Expr->Value.Create  = (CreateT*)  Ptr; break;
    case Nil:     Expr->Value.Nil     = (NilT*)     Ptr; break;
  }
  return Expr;
}

LitT *createLit(TypeKind K, void *Ptr) {
  LitT *Lit = (LitT*) malloc(sizeof(LitT));
  Lit->Kind = K;
  switch (K) {
    case Int:    Lit->Value.Int = *((int*) Ptr);     break;
    case Float:  Lit->Value.Float = *((float*) Ptr); break;
    case String: Lit->Value.String = (char*) Ptr;    break;
  }
  return Lit;
}

LvalT *createLval(LvalKind K, char *I, LvalT *Val, ExprT *E) {
  LvalT *Lval = (LvalT*) malloc(sizeof(LvalT));
  Lval->Kind = K;
  switch (K) {
    case Id: Lval->Id = I; break;
    case ArrayAccess:
      Lval->Lval = Val;
      Lval->Pos = E;
      break;
    case RecordAccess:
      Lval->Id = I;
      Lval->Lval = Val;
      break;
  }
  return Lval;
}

BinOpT *createBinOp(OpKind K, ExprT *E1, ExprT *E2) {
  BinOpT *BinOp = (BinOpT*) malloc(sizeof(BinOpT));
  BinOp->Kind = K;
  BinOp->ExprOne = E1;
  BinOp->ExprTwo = E2;
  return BinOp;
}

NegT *createNeg(ExprT *E) {
  NegT *Neg = (NegT*) malloc(sizeof(NegT));
  Neg->Expr = E;
  return Neg;
}

LetT *createLet(DeclT *D, ExprT *E) {
  LetT *Let = (LetT*) malloc(sizeof(LetT));
  Let->Decl = D;
  Let->Expr = E;
  return Let;
}

DeclT *createDecl(DeclKind K, void *Ptr, DeclT *N) {
  DeclT *Decl = (DeclT*) malloc(sizeof(DeclT));
  Decl->Kind = K;
  Decl->Next = N;
  switch (K) {
    case Var: Decl->Value.Var = (VarDeclT*) Ptr; break;
    case Fun: Decl->Value.Fun = (FunDeclT*) Ptr; break;
    case Ty:  Decl->Value.Ty  = (TyDeclT*)  Ptr; break;
  }
  return Decl;
}

TypeT *createType(TypeKind K, void *Ptr) {
  TypeT *Type = (TypeT*) malloc(sizeof(TypeT));
  Type->Kind = K;
  switch (K) {
    case Id:      Type->Ty.Id      = (char*)     Ptr; break;
    case FunTy:   Type->Ty.FunTy   = (FunTyT*)   Ptr; break;
    case ParamTy: Type->Ty.ParamTy = (ParamTyT*) Ptr; break;
    case ArrayTy: Type->Ty.ArrayTy = (ArrayTyT*) Ptr; break;
  }
  return Type;
}

ParamTyT *createParamTy(char *I, TypeT *Ty, ParamTyT *PNext) {
  ParamTyT *Params = (ParamTyT*) malloc(sizeof(ParamTyT));
  Params->Id = I;
  Params->Type = Ty;
  Params->Next = PNext;
  return Params;
}

VarDeclT *createVarDecl(char *I, ExprT *E) {
  VarDeclT *Var = (VarDeclT*) malloc(sizeof(VarDeclT));
  Var->Id = I;
  Var->Expr = E;
  return Var;
}

FunDeclT *createFunDecl(char *I, ExprT *E, ParamTyT *Param, FunDeclT *FNext) {
  FunDeclT *Fun = (FunDeclT*) malloc(sizeof(FunDeclT));
  Fun->Id = I;
  Fun->Expr = E;
  Fun->Params = Param;
  Fun->Next = FNext;
  return Fun;
}

TyDeclT *createTyDecl(char *I, TypeT *Type) {
  TyDeclT *Ty = (TyDeclT*) malloc(sizeof(TyDeclT));
  Ty->Id = I;
  Ty->Type = Type; 
  return Ty;
}

ArrayTyT *createArrayTy(TypeT *T) {
  ArrayTyT *ArrayTy = (ArrayTyT*) malloc(sizeof(ArrayTy));
  ArrayTy->Type = T;
  return ArrayTy;
}

SeqTyT *createSeqTy(TypeT *T, SeqTyT *Seq) {
  SeqTyT *SeqTy = (SeqTyT*) malloc(sizeof(SeqTy));
  SeqTy->Type = T;
  SeqTy->Next = Seq;
  return SeqTy;
}

FunTyT *createFunTy(SeqTyT *S1, SeqTyT *S2) {
  FunTyT *FunTy = (FunTyT*) malloc(sizeof(FunTyT));
  FunTy->From = S1;
  FunTy->To   = S2;
  return FunTy;
}

IfStmtT *createIfStmt(ExprT *Cond, ExprT *ThenBody, ExprT *ElseBody) {
  IfStmtT *If = (IfStmtT*) malloc(sizeof(IfStmtT));
  If->If = Cond;
  If->Then = ThenBody;
  If->Else = ElseBody;
  return If;
}

ArgT *createArg(ExprT *E, ArgT *N) {
  ArgT *Arg = (ArgT*) malloc(sizeof(ArgT));
  Arg->Expr = E;
  Arg->Next = N;
  return Arg;
}

FunCallT *createFunCall(char *I, ArgT *A) {
  FunCallT *FunCall = (FunCallT*) malloc(sizeof(FunCallT)); 
  FunCall->Id = I;
  FunCall->Args = A;
  return FunCall;
}

CreateT *createCreate(CreateKind K, void *Ptr) {
  CreateT *Create = (CreateT*) malloc(sizeof(CreateT));
  Create->Kind = K;
  switch (K) {
    case Array:  Create->Value.Array  = (ArrayT*)  Ptr; break;
    case Record: Create->Value.Record = (RecordT*) Ptr; break;
  }
  return Create;
}

ArrayT *createArray(char *I, ExprT *S, ExprT *V) {
  ArrayT *Array = (ArrayT*) malloc(sizeof(ArrayT));
  Array->Id = I;
  Array->Size = S;
  Array->Value = V;
  return Array;
}

RecordT *createRecord(char *I, FieldT *Fi) {
  RecordT *Record = (RecordT*) malloc(sizeof(RecordT));
  Record->Id = I;
  Record->Field = Fi;
  return Record;
}

FieldT *createField(char *I, ExprT *E, FieldT *N) {
  FieldT *Field = (FieldT*) malloc(sizeof(FieldT));
  Field->Id = I;
  Field->Expr = E;
  Field->Next = N;
  return Field;
}
