#include "ftc/LLVMCode/Translate.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

void toFunctionName(char *Dst, char *TypeName) {
  sprintf(Dst, "function.%s", TypeName);
}

void toValName(char *Dst, char *TypeName) {
  sprintf(Dst, "val.%s", TypeName);
}

void toStructName(char *Dst, char *TypeName) {
  sprintf(Dst, "struct.%s", TypeName);
}

void *getAliasValue(SymbolTable *Table, char *NodeId, void (*toName)(char*,char*), 
    void *(*tableFind)(SymbolTable*, char*)) {
  char Buf[NAME_MAX], *Name;
  (*toName)(Buf, NodeId);
  Name = (char*) symTableFind(Table, Buf);
  return (*tableFind)(Table, Name);
}

static char *pickInsertAlias(SymbolTable *Table, char *NodeId, void (*toBaseName)(char*,char*),
    int (*symbolExists)(SymbolTable*, char*)) {
  int Count = 0;
  char Buf[NAME_MAX], *BaseName, *Name;
  do {
    (*toBaseName)(Buf, NodeId);
    if (!BaseName) {
      BaseName = (char*) malloc(sizeof(char) * strlen(Buf));
      strcpy(BaseName, Buf);
    }
    sprintf(Buf, "%s%d", Buf, Count++);
  } while ((*symbolExists)(Table, Buf));

  Name = (char*) malloc(sizeof(char) * strlen(Buf));
  strcpy(Name, Buf);

  symTableInsert(Table, BaseName, Name);
  return Name;
}

LLVMTypeRef getLLVMTypeFromType(SymbolTable *St, Type *Ty) {
  if (!Ty) return NULL;
  Type *RealType = getRealType(St, Ty);

  LLVMTypeRef TypeRef = NULL;
  switch (RealType->Kind) {
    case IntTy:
      TypeRef = LLVMInt32Type();
      break;
    case FloatTy:
      TypeRef = LLVMFloatType();
      break;
    case StringTy:
      TypeRef = LLVMPointerType(LLVMInt8Type(), 0);
      break;
    case FunTy:
      {
        char Buf[] = "struct.Closure";
        TypeRef = symTableFind(St, Buf);
        break;
      }
    case IdTy:
      {
        char Buf[NAME_MAX], *StrName;
        Type *Detailed = (Type*) symTableFind(St, RealType->Val);
        if (Detailed->Kind == RecordTy) {
          toStructName(Buf, RealType->Val);
          StrName = (char*) symTableFind(St, Buf);
          // Get pointer to struct.
          TypeRef = (LLVMTypeRef) symTableFindGlobal(St, StrName);
        } else if (Detailed->Kind == ArrayTy) {
          LLVMTypeRef ElemTypeRef = getLLVMTypeFromType(St, RealType->Val);
          TypeRef = LLVMPointerType(ElemTypeRef, 0);
        }
        break;
      }
    default:
      break;
  }
  return TypeRef;
}

LLVMValueRef toDynamicMemory(LLVMValueRef Val) {
  LLVMValueRef ValLoad   = LLVMBuildLoad(Builder, Val, "");
  LLVMTypeRef  ValueType = LLVMTypeOf(ValLoad);

  LLVMValueRef DynMemPtr = LLVMBuildMalloc(Builder, ValueType, "");

  LLVMTargetDataRef DataRef = LLVMCreateTargetData(LLVMGetDataLayout(Module));
  unsigned long long Size   = LLVMStoreSizeOfType(DataRef, ValueType);

  copyMemory(DynMemPtr, Val, Size);
  return DynMemPtr;
}

LLVMValueRef getEscapedVar(SymbolTable *ValTable, Type *VarType, ASTNode *Node) {
  int I, Offset = findEscapedOffset(ValTable, Node->Value, VarType->EscapedLevel);
  LLVMValueRef RA        = getHeadRA();
  LLVMValueRef RAContent = LLVMBuildLoad(Builder, RA, "");

  LLVMValueRef StaticLinkIdx[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 1),
    LLVMConstInt(LLVMInt32Type(), 1, 1)
  };
  for (I = 1; I < VarType->EscapedLevel; ++I) {
    RA        = LLVMBuildInBoundsGEP(Builder, RAContent, StaticLinkIdx, 2, "");
    RAContent = LLVMBuildLoad(Builder, RA, "");
  }

  LLVMValueRef EscapedVarsIdx[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 1),
    LLVMConstInt(LLVMInt32Type(), 0, 1),
  };
  LLVMValueRef EscapedVarsPtr = LLVMBuildInBoundsGEP(Builder, RAContent, EscapedVarsIdx, 2, "");
  LLVMValueRef EscapedVars    = LLVMBuildLoad(Builder, EscapedVarsPtr, "");

  LLVMValueRef VarOffsetIdx[] = {
    LLVMConstInt(LLVMInt32Type(), Offset, 1)
  };
  LLVMValueRef EscapedVar = LLVMBuildInBoundsGEP(Builder, EscapedVars, VarOffsetIdx, 1, "");
  /*
   *  Return a <type>**
   */
  return EscapedVar;
}

static void translateVarDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);

  ASTNode *ExprNode = (ASTNode*) ptrVectorGet(V, 1);
  Type    *VarType  = (Type*) symTableFind(ValTable, Node->Value),
          *RealType = getRealType(TyTable, VarType);

  LLVMTypeRef  VarTyRef = getLLVMTypeFromType(TyTable, RealType);
  LLVMValueRef Expr     = translateExpr(TyTable, ValTable, ExprNode),
               Val, Variable;

  char Buf[NAME_MAX],
       *Name = pickInsertAlias(ValTable, Node->Value, &toValName, &symTableExistsLocal);

  if (RealType->Kind == IntTy || RealType->Kind == FloatTy) {
    Val = Expr; 
    Variable = LLVMBuildAlloca(Builder, VarTyRef, Name);
    LLVMBuildStore(Builder, Val, Variable);
  } else {
    if (RealType->Kind == IdTy) 
      RealType = (Type*) symTableFind(TyTable, RealType->Val);
    if (RealType->Kind == ArrayTy || RealType->Kind == StringTy) {
      LLVMValueRef String = LLVMBuildLoad(Builder, Expr, "");
      Val = LLVMBuildBitCast(Builder, String, VarTyRef, "");
    }

    strcpy(Buf, "pre.");
    strcat(Buf, Name);
    LLVMValueRef PreVar = LLVMBuildAlloca(Builder, LLVMPointerType(VarTyRef, 0), Buf);
    LLVMBuildStore(Builder, Val, PreVar);
    Variable = LLVMBuildLoad(Builder, PreVar, Name);
  }

  symTableInsertLocal(ValTable, Name, Variable);

  if (VarType->EscapedLevel > 0) {
    LLVMValueRef EscV = getEscapedVar(ValTable, VarType, Node);
    LLVMValueRef EscVarCast = LLVMBuildBitCast(Builder, EscV, VarTyRef, "");
    LLVMBuildStore(Builder, toDynamicMemory(Variable), EscVarCast);
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

    LLVMTypeRef *ParamsTyRef;
    if (V->Size)
      ParamsTyRef = (LLVMTypeRef*) malloc(sizeof(LLVMTypeRef) * V->Size);

    PtrVectorIterator I = beginPtrVector(V),
                      E = endPtrVector(V);
    for (; I != E; ++I) 
      ParamsTyRef[*Count++] = getLLVMTypeFromType(St, *I);
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
                ReturnTypeRef = getLLVMTypeFromType(TyTable, ArrTy[1]),
                FunTypeRef = LLVMFunctionType(ReturnTypeRef, ParamTypeRef, Size, 0);

    // Creating alias for the function
    char *Name   = pickInsertAlias(ValTable, FnDeclNode->Value, &toValName, &symTableExistsGlobal);
    char *FnName = pickInsertAlias(ValTable, FnDeclNode->Value, &toFunctionName, &symTableExistsGlobal);

    // Creating new function, and its entry BasicBlock.
    LLVMValueRef      Function = LLVMAddFunction(Module, Name, FunTypeRef);

    // Activation record and closure created.
    LLVMValueRef ThisRA = createActivationRecord(Builder, ValTable, FnDeclNode);
    LLVMValueRef ThisClosure = createLocalClosure(Builder, Function, ThisRA);

    // Inserting the function name alias in the hash.
    symTableInsert(ValTable, Name, ThisClosure);
    symTableInsertGlobal(ValTable, FnName, FunTypeRef);

    if (FunType->EscapedLevel > 0) {
      LLVMValueRef EscVar     = getEscapedVar(ValTable, FunType, FnDeclNode);
      LLVMValueRef EscVarCast = LLVMBuildBitCast(Builder, EscVar, FunTypeRef, "");
      LLVMBuildStore(Builder, toDynamicMemory(ThisClosure), EscVarCast);
    }   
  }
  I = beginPtrVector(&(Node->Child));
  E = endPtrVector(&(Node->Child));
  for (; I != E; ++I) {
    translateDecl(TyTable, ValTable, *I);
  }
}

static void translateFunDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  SymbolTable *TyTable_  = symTableFindChild(TyTable, Node),
              *ValTable_ = symTableFindChild(ValTable, Node);
  ASTNode *Expr    = (ASTNode*) ptrVectorGet(&(Node->Child), 2); 
  Type    *FunType = (Type*) symTableFind(ValTable, Node->Value),
          **ArrTy  = (Type**) FunType->Val;

  // Putting RAHead forward
  LLVMValueRef Closure = (LLVMValueRef) getAliasValue(ValTable, Node->Value, &toValName, &symTableFind);
  LLVMValueRef RAIdx[] = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef ClosRA  = LLVMBuildInBoundsGEP(Builder, Closure, RAIdx, 2, "");
  LLVMValueRef RALoad  = LLVMBuildLoad(Builder, ClosRA, "");
  putRAHeadAhead(RALoad);

  // Getting function LLVMValueRef
  LLVMValueRef FnIdx[]  = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef ClosFn   = LLVMBuildInBoundsGEP(Builder, Closure, FnIdx, 2, "");
  LLVMValueRef Function = LLVMBuildLoad(Builder, ClosFn, "");

  // Saving the last BasicBlock.
  LLVMBasicBlockRef OldBB = LLVMGetInsertBlock(Builder);

  // Creating new BasicBlock
  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef ReturnVal = translateExpr(TyTable_, ValTable_, Expr);
  if (ArrTy[1]->Kind == AnswerTy) LLVMBuildRetVoid(Builder);
  else if (ArrTy[1]->Kind != IntTy || ArrTy[1]->Kind != FloatTy) {
    ReturnVal = toDynamicMemory(ReturnVal);
  }
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
