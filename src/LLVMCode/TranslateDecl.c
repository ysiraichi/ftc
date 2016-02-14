#include "ftc/LLVMCode/Translate.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

static void translateVarDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("VarDecl\n");
  PtrVector *V = &(Node->Child);

  ASTNode *ExprNode = (ASTNode*) ptrVectorGet(V, 1);
  Type    *VarType  = (Type*) symTableFind(ValTable, Node->Value);

  LLVMValueRef ExprVal  = translateExpr(TyTable, ValTable, ExprNode);

  char *Name = pickInsertAlias(ValTable, Node->Value, &toValName, &symTableExistsLocal);

  symTableInsertLocal(ValTable, Name, ExprVal);

  if (VarType->EscapedLevel > 0) {
    LLVMValueRef EscV = getEscapedVar(ValTable, Node->Value, Node->EscapedLevel);
    LLVMValueRef EscVarCast = LLVMBuildBitCast(Builder, EscV, LLVMPointerType(LLVMTypeOf(ExprVal), 0), "");
    LLVMBuildStore(Builder, toDynamicMemory(ExprVal), EscVarCast);
  }
}

static LLVMTypeRef *getLLVMStructField(SymbolTable *TyTable, ASTNode *Node, int *Size) {
  *Size = 0;
  Type *RecordType = getRecordType(TyTable, symTableFind(TyTable, Node->Value));
  if (RecordType) {
    Hash      *H = (Hash*) RecordType->Val;
    PtrVector *V = &(H->Pairs);

    LLVMTypeRef *Fields;
    if (V->Size)
      Fields = (LLVMTypeRef*) malloc(sizeof(LLVMTypeRef) * V->Size);

    PtrVectorIterator I = beginPtrVector(V),
                      E = endPtrVector(V);
    for (; I != E; ++I) {
      Pair *P = (Pair*) *I;
      Fields[(*Size)++] = 
        wrapStructElementType(getLLVMTypeFromType(TyTable, P->second));
    }
    return Fields;
  }
  return NULL;
}

static void translateTyDeclList(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("TyDeclList\n");
  PtrVectorIterator I, E;
  I = beginPtrVector(&(Node->Child));
  E = endPtrVector(&(Node->Child));
  for (; I != E; ++I) {
    ASTNode *TyDeclNode = (ASTNode*) *I;

    Type *RecordType = getRecordType(TyTable, symTableFind(TyTable, TyDeclNode->Value));
    if (!RecordType) continue;

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
  printf("TyDecl\n");
  int Size;
  LLVMTypeRef *Fields = getLLVMStructField(TyTable, Node, &Size);
  if (Size) {
    char *Name = getAliasName(TyTable, Node->Value, &toStructName);
    LLVMTypeRef Struct = LLVMGetTypeByName(Module, Name);
    LLVMStructSetBody(Struct, Fields, Size, 0);
  }
}

static LLVMTypeRef *getLLVMTypesFromSeqTy(SymbolTable *St, Type *ParamType, int *Count) {
  LLVMTypeRef *SeqTyRef = NULL;
  if (!ParamType) return NULL;
  if (ParamType->Kind == SeqTy) {
    *Count = 0;
    PtrVector *V = (PtrVector*) ParamType->Val;

    if (V->Size)
      SeqTyRef = (LLVMTypeRef*) malloc(sizeof(LLVMTypeRef) * V->Size);

    PtrVectorIterator I = beginPtrVector(V),
                      E = endPtrVector(V);
    for (; I != E; ++I) {
      LLVMTypeRef ParamType = getLLVMTypeFromType(St, *I);

      SeqTyRef[(*Count)++] = toTransitionType(ParamType);
    }
  }
  return SeqTyRef;
}

static void translateFunDeclList(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("FunDeclList\n");
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

    // Creating function
    LLVMAddFunction(Module, Name, FunTypeRef);

  }
  I = beginPtrVector(&(Node->Child));
  E = endPtrVector(&(Node->Child));
  for (; I != E; ++I) {
    translateDecl(TyTable, ValTable, *I);
  }
}

static void translateArgs(SymbolTable *ValTable, ASTNode *Node, LLVMValueRef Function) {
  printf("Args\n");
  if (!Node) return;

  unsigned I;
  PtrVector *V = &(Node->Child);

  for (I = 0; I < V->Size; ++I) {
    ASTNode *ArgNode = (ASTNode*) ptrVectorGet(V, I);
    Type    *ArgType = (Type*) symTableFind(ValTable, ArgNode->Value);
    char    *Name    = pickInsertAlias(ValTable, ArgNode->Value, &toValName, &symTableExistsLocal);

    LLVMValueRef Param     = LLVMGetParam(Function, I);
    LLVMTypeRef  ParamType = LLVMTypeOf(Param);
    printf("Translate: Inserted parameter '%s' at %p.\n", Name, (void*) ValTable);
    symTableInsertLocal(ValTable, Name, wrapValue(Param));

    if (ArgType->EscapedLevel > 0) {
      printf("Translate: Updating '%s' with 'hasEscaped':%d.\n", (char*)ArgNode->Value, ArgType->EscapedLevel);
      LLVMValueRef Wrapped    = wrapValue(Param);
      LLVMTypeRef  WrappedTy  = LLVMTypeOf(Wrapped);
      LLVMValueRef EscV       = getEscapedVar(ValTable, ArgNode->Value, ArgNode->EscapedLevel);
      LLVMValueRef EscVarCast = LLVMBuildBitCast(Builder, EscV, LLVMPointerType(WrappedTy, 0), "");
      LLVMBuildStore(Builder, toDynamicMemory(Wrapped), EscVarCast);
    }
  }
}

static void translateFunDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("FunDecl\n");
  SymbolTable *TyTable_  = symTableFindChild(TyTable, Node),
              *ValTable_ = symTableFindChild(ValTable, Node);
  ASTNode *Expr    = (ASTNode*) ptrVectorGet(&(Node->Child), 2); 
  Type    *FunType = (Type*) symTableFind(ValTable, Node->Value),
          **ArrTy  = (Type**) FunType->Val;

  // Getting function LLVMValueRef
  LLVMValueRef Function = getAliasFunction(ValTable, Node->Value, &toValName);

  // Activation record and closure created.
  putRAHeadAhead(LLVMBuildLoad(Builder, getHeadRA(), "ld.heap.ra"));
  LLVMValueRef ThisClosure = createClosure(Builder, Function, createActivationRecordWithSl(Builder, Node->Value));
  returnRAHead();

  // Inserting the function name alias in the hash.
  char *Name = getAliasName(ValTable, Node->Value, &toValName);
  symTableInsertLocal(ValTable, Name, ThisClosure);
  printf("Translate: Inserted function '%s' into %p.\n", Name, (void*)ValTable);

  if (FunType->EscapedLevel > 0) {
    LLVMValueRef EscVariable = getEscapedVar(ValTable, Node->Value, Node->EscapedLevel);
    LLVMValueRef EscClosCast = LLVMBuildBitCast(Builder, ThisClosure, LLVMPointerType(LLVMInt8Type(), 0), "");
    LLVMBuildStore(Builder, EscClosCast, EscVariable);
  }

  // Saving the last BasicBlock.
  LLVMBasicBlockRef OldBB = LLVMGetInsertBlock(Builder);

  /* =--------------------------------------- Inside new function ------------------------------------------- */

  // Creating entry BasicBlock.
  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  // Creating activation record.
  LLVMValueRef ThisRA = LLVMBuildLoad(Builder, getHeadRA(), "ld.heap.ra");
  createDataLink(Builder, ThisRA, ValTable_, Node->Value);

  // Getting function type.
  int Size = 0;
  LLVMTypeRef *ParamTypeRef = getLLVMTypesFromSeqTy(TyTable, ArrTy[0], &Size),
              ReturnTypeRef = toTransitionType(getLLVMTypeFromType(TyTable, ArrTy[1])),
              FunTypeRef = LLVMFunctionType(ReturnTypeRef, ParamTypeRef, Size, 0);
  FunTypeRef = LLVMPointerType(FunTypeRef, 0);

  translateArgs(ValTable_, ptrVectorGet(&(Node->Child), 0), Function);

  LLVMValueRef ReturnVal = translateExpr(TyTable_, ValTable_, Expr);

  if (ArrTy[1]->Kind == AnswerTy) LLVMBuildRetVoid(Builder);
  else {
    ReturnVal = unWrapValue(ReturnVal);
    LLVMBuildRet(Builder, ReturnVal);
  }
  /* =------------------------------------------------------------------------------------------------------- */

  // Restoring the BasicBlock saved.
  if (OldBB) LLVMPositionBuilderAtEnd(Builder, OldBB);
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
