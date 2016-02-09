#include "ftc/LLVMCode/Translate.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

static void translateVarDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);

  ASTNode *ExprNode = (ASTNode*) ptrVectorGet(V, 1);
  Type    *VarType  = (Type*) symTableFind(ValTable, Node->Value),
          *RealType = getRealType(TyTable, VarType);

  LLVMTypeRef  VarTyRef = getLLVMTypeFromType(TyTable, RealType);
  LLVMValueRef ExprVal  = translateExpr(TyTable, ValTable, ExprNode);

  char *Name = pickInsertAlias(ValTable, Node->Value, &toValName, &symTableExistsLocal);

  symTableInsertLocal(ValTable, Name, ExprVal);

  if (VarType->EscapedLevel > 0) {
    LLVMValueRef EscV = getEscapedVar(ValTable, VarType, Node);
    LLVMValueRef EscVarCast = LLVMBuildBitCast(Builder, EscV, VarTyRef, "");
    LLVMBuildStore(Builder, toDynamicMemory(ExprVal), EscVarCast);
  }
}

static LLVMTypeRef *getLLVMStructField(SymbolTable *TyTable, ASTNode *Node, int *Size) {
  *Size = 0;
  Type    *RecordType = getRealType(TyTable, symTableFind(TyTable, Node->Value));
  if (RecordType->Kind == IdTy) 
    RecordType = (Type*) symTableFind(TyTable, RecordType->Val);

  if (RecordType->Kind == RecordTy) {
    Hash      *H = (Hash*) RecordType->Val;
    PtrVector *V = &(H->Pairs);

    LLVMTypeRef *Fields;
    if (V->Size)
      Fields = (LLVMTypeRef*) malloc(sizeof(LLVMTypeRef) * V->Size);

    PtrVectorIterator I = beginPtrVector(V),
                      E = endPtrVector(V);
    for (; I != E; ++I)
      Fields[*Size++] = getLLVMTypeFromType(TyTable, *I);
    return Fields;
  }
  return NULL;
}

static void translateTyDeclList(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVectorIterator I, E;
  I = beginPtrVector(&(Node->Child));
  E = endPtrVector(&(Node->Child));
  for (; I != E; ++I) {
    ASTNode *TyDeclNode = (ASTNode*) *I;
    Type    *RecordType = getRealType(TyTable, symTableFind(TyTable, TyDeclNode->Value));
    if (RecordType->Kind == IdTy) 
      RecordType = (Type*) symTableFind(TyTable, RecordType->Val);
    if (RecordType->Kind != RecordTy) continue;

    LLVMContextRef Context = LLVMGetModuleContext(Module);

    char *Name = pickInsertAlias(TyTable, TyDeclNode->Value, &toStructName, &symTableExistsGlobal);
    LLVMTypeRef Struct = LLVMStructCreateNamed(Context, Name);

    symTableInsertGlobal(TyTable, Name, Struct);
  }

  I = beginPtrVector(&(Node->Child));
  E = endPtrVector(&(Node->Child));
  for (; I != E; ++I) {
    translateDecl(TyTable, ValTable, *I);
  }
}

static void translateTyDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  int Size;
  LLVMTypeRef *Fields = getLLVMStructField(TyTable, Node, &Size);
  if (Size) {
    LLVMTypeRef Struct = getLLVMTypeFromType(TyTable, createType(Node->Kind, Node->Value));
    LLVMStructSetBody(Struct, Fields, Size, 0);
  }
}

static LLVMTypeRef *getLLVMTypesFromSeqTy(SymbolTable *St, Type *ParamType, int *Count) {
  LLVMTypeRef *SeqTyRef = NULL;
  if (ParamType->Kind == SeqTy) {
    *Count = 0;
    PtrVector *V = (PtrVector*) ParamType->Val;

    if (V->Size)
      SeqTyRef = (LLVMTypeRef*) malloc(sizeof(LLVMTypeRef) * V->Size);

    PtrVectorIterator I = beginPtrVector(V),
                      E = endPtrVector(V);
    for (; I != E; ++I) {
      LLVMTypeRef ParamType = getLLVMTypeFromType(St, *I);

      SeqTyRef[*Count++] = toTransitionType(ParamType);
    }
  }
  return SeqTyRef;
}

static void translateFunDeclList(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVectorIterator I, E;
  I = beginPtrVector(&(Node->Child));
  E = endPtrVector(&(Node->Child));
  for (; I != E; ++I) {
    ASTNode *FnDeclNode = (ASTNode*) *I;

    Type *FunType   = (Type*) symTableFind(ValTable, FnDeclNode->Value),
         **ArrTy    = (Type**) FunType->Val;

    // Getting function parameters.
    int Size = 0;
    LLVMTypeRef *ParamTypeRef = getLLVMTypesFromSeqTy(TyTable, ArrTy[0], &Size),
                ReturnTypeRef = toTransitionType(getLLVMTypeFromType(TyTable, ArrTy[1])),
                FunTypeRef = LLVMFunctionType(ReturnTypeRef, ParamTypeRef, Size, 0);

    // Creating alias for the function
    char *Name   = pickInsertAlias(ValTable, FnDeclNode->Value, &toValName, &symTableExistsGlobal);

    // Creating new function, and its entry BasicBlock.
    LLVMValueRef      Function = LLVMAddFunction(Module, Name, FunTypeRef);

    // Activation record and closure created.
    LLVMValueRef ThisRA = createActivationRecord(Builder, ValTable, FnDeclNode->Value);
    LLVMValueRef ThisClosure = createLocalClosure(Builder, Function, ThisRA);

    // Inserting the function name alias in the hash.
    symTableInsert(ValTable, Name, ThisClosure);

    if (FunType->EscapedLevel > 0) {
      LLVMValueRef EscVar     = getEscapedVar(ValTable, FunType, FnDeclNode);
      LLVMValueRef EscVarCast = LLVMBuildBitCast(Builder, EscVar, LLVMPointerType(FunTypeRef, 0), "");
      LLVMBuildStore(Builder, toDynamicMemory(ThisClosure), EscVarCast);
    }   
  }
  I = beginPtrVector(&(Node->Child));
  E = endPtrVector(&(Node->Child));
  for (; I != E; ++I) {
    translateDecl(TyTable, ValTable, *I);
  }
}

static void translateArgs(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node,
    LLVMValueRef Function) {

  unsigned I;
  PtrVector *V = &(Node->Child);

  for (I = 0; I < V->Size; ++I) {
    ASTNode *ArgNode = (ASTNode*) ptrVectorGet(V, I);
    Type    *ArgType = (Type*) symTableFind(TyTable, ArgNode->Value);
    char    *Name    = pickInsertAlias(ValTable, ArgNode->Value, &toValName, &symTableExistsLocal);

    LLVMValueRef Param     = LLVMGetParam(Function, I);
    LLVMTypeRef  ParamType = LLVMTypeOf(Param);
    symTableInsertLocal(ValTable, Name, Param);

    if (ArgType->EscapedLevel > 0) {
      LLVMValueRef EscV       = getEscapedVar(ValTable, ArgType, ArgNode);
      LLVMValueRef EscVarCast = LLVMBuildBitCast(Builder, EscV, LLVMPointerType(ParamType, 0), "");
      LLVMBuildStore(Builder, toDynamicMemory(Param), EscVarCast);
    }
  }
}

static void translateFunDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  SymbolTable *TyTable_  = symTableFindChild(TyTable, Node),
              *ValTable_ = symTableFindChild(ValTable, Node);
  ASTNode *Expr    = (ASTNode*) ptrVectorGet(&(Node->Child), 2); 
  Type    *FunType = (Type*) symTableFind(ValTable, Node->Value),
          **ArrTy  = (Type**) FunType->Val;

  // Putting RAHead forward
  LLVMValueRef Closure = (LLVMValueRef) resolveAliasId(ValTable, Node->Value, &toValName, &symTableFind);
  LLVMValueRef RAIdx[] = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef ClosRA  = LLVMBuildInBoundsGEP(Builder, Closure, RAIdx, 2, "");
  LLVMValueRef RALoad  = LLVMBuildLoad(Builder, ClosRA, "");
  putRAHeadAhead(RALoad);

  // Getting function LLVMValueRef
  LLVMValueRef FnIdx[]  = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef ClosFn   = LLVMBuildInBoundsGEP(Builder, Closure, FnIdx, 2, "");
  LLVMValueRef Function = LLVMBuildLoad(Builder, ClosFn, "");

  translateArgs(TyTable_, ValTable_, ptrVectorGet(&(Node->Child), 0), Function);

  // Saving the last BasicBlock.
  LLVMBasicBlockRef OldBB = LLVMGetInsertBlock(Builder);

  // Creating new BasicBlock
  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef ReturnVal = translateExpr(TyTable_, ValTable_, Expr);
  if (ArrTy[1]->Kind == AnswerTy) LLVMBuildRetVoid(Builder);
  LLVMBuildRet(Builder, ReturnVal);

  // Restoring the BasicBlock saved.
  if (OldBB) LLVMPositionBuilderAtEnd(Builder, OldBB);

  // Restoring RAHead
  returnRAHead();
}

void translateDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVectorIterator I = beginPtrVector(&(Node->Child)),
                    E = endPtrVector(&(Node->Child));
  switch (Node->Kind) {
    case DeclList:    for (; I != E; ++I) translateDecl(TyTable, ValTable, *I); break;
    case TyDeclList:  translateTyDeclList (TyTable, ValTable, Node); break;
    case FunDeclList: translateFunDeclList(TyTable, ValTable, Node); break;

    case VarDecl: translateVarDecl(TyTable, ValTable, Node); break;
    case TyDecl:  translateTyDecl (TyTable, ValTable, Node); break;
    case FunDecl: translateFunDecl(TyTable, ValTable, Node); break;

    default: break;
  }
}
