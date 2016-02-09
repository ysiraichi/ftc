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
  printf("TyDeclList\n");
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
  printf("TyDecl\n");
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

    SymbolTable *ValTable_ = symTableFindChild(ValTable, FnDeclNode);

    Type *FunType   = (Type*) symTableFind(ValTable, FnDeclNode->Value),
         **ArrTy    = (Type**) FunType->Val;

    // Getting function parameters.
    int Size = 0;
    LLVMTypeRef *ParamTypeRef = getLLVMTypesFromSeqTy(TyTable, ArrTy[0], &Size),
                ReturnTypeRef = toTransitionType(getLLVMTypeFromType(TyTable, ArrTy[1])),
                FunTypeRef = LLVMFunctionType(ReturnTypeRef, ParamTypeRef, Size, 0);

    // Creating alias for the function
    char *Name   = pickInsertAlias(ValTable, FnDeclNode->Value, &toValName, &symTableExistsGlobal);

    // Saving the last BasicBlock.
    LLVMBasicBlockRef OldBB = LLVMGetInsertBlock(Builder);

    // Creating new function, and its entry BasicBlock.
    LLVMValueRef   Function = LLVMAddFunction(Module, Name, FunTypeRef);
    LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
    LLVMPositionBuilderAtEnd(Builder, Entry);

    // Activation record and closure created.
    LLVMValueRef ThisRA = createActivationRecord(Builder, ValTable_, FnDeclNode->Value);
    LLVMValueRef ThisClosure = createLocalClosure(Builder, Function, ThisRA);

    // Inserting the function name alias in the hash.
    printf("Translate: Inserted function '%s' into %p.\n", Name, (void*)ValTable);
    symTableInsertLocal(ValTable, Name, ThisClosure);

    if (FunType->EscapedLevel > 0) {
      LLVMValueRef EscClosure  = toDynamicMemory(ThisClosure);
      LLVMValueRef EscVariable = getEscapedVar(ValTable_, FunType, FnDeclNode);
      LLVMValueRef EscClosCast = LLVMBuildBitCast(Builder, EscClosure, LLVMPointerType(LLVMInt8Type(), 0), "");
      LLVMBuildStore(Builder, EscClosCast, EscVariable);
    }   

    // Restoring the BasicBlock saved.
    if (OldBB) LLVMPositionBuilderAtEnd(Builder, OldBB);
  }
  I = beginPtrVector(&(Node->Child));
  E = endPtrVector(&(Node->Child));
  for (; I != E; ++I) {
    translateDecl(TyTable, ValTable, *I);
  }
}

static void translateArgs(SymbolTable *ValTable, ASTNode *Node, LLVMValueRef Function) {
  printf("Args\n");

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
      LLVMValueRef EscV       = getEscapedVar(ValTable, ArgType, ArgNode);
      LLVMValueRef EscVarCast = LLVMBuildBitCast(Builder, EscV, LLVMPointerType(ParamType, 0), "");
      LLVMBuildStore(Builder, toDynamicMemory(Param), EscVarCast);
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

  // Saving the last BasicBlock.
  LLVMBasicBlockRef OldBB = LLVMGetInsertBlock(Builder);

  // Getting function LLVMValueRef
  LLVMValueRef Function = getAliasFunction(ValTable, Node->Value, &toValName);

  // Creating new BasicBlock
  LLVMBasicBlockRef Entry = LLVMGetEntryBasicBlock(Function);
  LLVMPositionBuilderAtEnd(Builder, Entry);

  // Getting function closure.
  LLVMValueRef Closure = (LLVMValueRef) resolveAliasId(ValTable, Node->Value, &toValName, &symTableFind);

  // Putting RAHead forward
  LLVMValueRef RAIdx[] = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef ClosRA  = LLVMBuildInBoundsGEP(Builder, Closure, RAIdx, 2, "");
  LLVMValueRef RALoad  = LLVMBuildLoad(Builder, ClosRA, "top.ra");
  putRAHeadAhead(RALoad);
  
  // Getting function type.
  int Size = 0;
  LLVMTypeRef *ParamTypeRef = getLLVMTypesFromSeqTy(TyTable, ArrTy[0], &Size),
              ReturnTypeRef = toTransitionType(getLLVMTypeFromType(TyTable, ArrTy[1])),
              FunTypeRef = LLVMFunctionType(ReturnTypeRef, ParamTypeRef, Size, 0);
  FunTypeRef = LLVMPointerType(FunTypeRef, 0);
  
  translateArgs(ValTable_, ptrVectorGet(&(Node->Child), 0), Function);

  // Restoring RAHead
  returnRAHead();

  LLVMValueRef ReturnVal = translateExpr(TyTable_, ValTable_, Expr);
  if (ArrTy[1]->Kind == AnswerTy) LLVMBuildRetVoid(Builder);
  else {
    ReturnVal = unWrapValue(ReturnVal);
    LLVMBuildRet(Builder, ReturnVal);
  }

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
