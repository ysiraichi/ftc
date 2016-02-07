#include "ftc/LLVMCode/Translate.h"

#include <stdio.h>
#include <string.h>

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;
extern int BBCount, GVCount;

static LLVMValueRef 
translateIntLit(ASTNode *Node) {
  return LLVMConstInt(LLVMInt32Type(), *((int*)Node->Value), 1); 
}

static LLVMValueRef 
translateFloatLit(ASTNode *Node) {
  return LLVMConstReal(LLVMFloatType(), *((float*)Node->Value));
}

static LLVMValueRef 
translateStringLit(ASTNode *Node) {
  int Length = strlen(Node->Value);
  char Buf[NAME_MAX];
  sprintf(Buf, "global.%d", GVCount++);

  LLVMValueRef GlobVar = LLVMAddGlobal(Module, LLVMArrayType(LLVMInt8Type(), Length+1), Buf);
  LLVMSetInitializer(GlobVar, LLVMConstString(Node->Value, Length, 0));

  LLVMValueRef LocalVar = LLVMBuildAlloca(Builder, LLVMArrayType(LLVMInt8Type(), Length+1), "");
  copyMemory(LocalVar, GlobVar, Length);
  return LocalVar;
}

static LLVMValueRef 
translateIdLval(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  Type *IdType = (Type*) symTableFind(ValTable, Node->Value);
  LLVMValueRef IdValue;
  if (IdType->EscapedLevel > 0) {
    LLVMTypeRef LLVMType = getLLVMTypeFromType(TyTable, IdType);
    LLVMValueRef Value = getEscapedVar(ValTable, IdType, Node);
    IdValue = LLVMBuildBitCast(Builder, Value, LLVMType, "");
  } else {
    char BaseName[NAME_MAX], *Name;
    toValName(BaseName, Node->Value);
    Name = (char*) symTableFind(ValTable, BaseName);
    IdValue = symTableFind(ValTable, Name); 
  }

  LLVMTypeRef IdElemType = LLVMGetElementType(LLVMTypeOf(IdValue));
  if (LLVMGetTypeKind(IdElemType) == LLVMIntegerTypeKind || LLVMGetTypeKind(IdElemType) == LLVMFloatTypeKind)
    IdValue = LLVMBuildLoad(Builder, IdValue, "");
  return IdValue;
}

static LLVMValueRef 
translateRecAccessLval(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  ASTNode *Lval = (ASTNode*) ptrVectorGet(&(Node->Child), 0);

  LLVMValueRef Record     = translateExpr(TyTable, ValTable, Lval);
  LLVMTypeRef  RecTypeRef = LLVMGetElementType(LLVMTypeOf(Record));

  const char  *Buf = LLVMGetStructName(RecTypeRef); 
  char         TypeName[NAME_MAX];
  strcpy(TypeName, Buf);

  Type      *RecType = (Type*) symTableFind(TyTable, TypeName);
  Hash      *H       = (Hash*) RecType->Val;
  PtrVector *V       = &(H->Pairs);

  unsigned Count = 0;
  for (Count = 0; Count < V->Size; ++Count) {
    Pair *P = (Pair*) ptrVectorGet(V, Count);
    if (!strcmp(P->first, Node->Value)) break;
  }

  LLVMValueRef Idx[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 1),
    LLVMConstInt(LLVMInt32Type(), Count, 1)
  };

  return LLVMBuildGEP(Builder, Record, Idx, 2, "");
}

static LLVMValueRef 
translateArrAccessLval(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  ASTNode *Lval  = (ASTNode*) ptrVectorGet(&(Node->Child), 0),
          *Index = (ASTNode*) ptrVectorGet(&(Node->Child), 1);

  LLVMValueRef Array  = translateExpr(TyTable, ValTable, Lval);
  LLVMValueRef IndVal = translateExpr(TyTable, ValTable, Index);
  LLVMValueRef Idx[] = { LLVMConstInt(LLVMInt32Type(), 0, 1), IndVal };
  return LLVMBuildGEP(Builder, Array, Idx, 2, ""); 
}

static LLVMValueRef
translateIntBinOp(NodeKind Op, LLVMValueRef ValueE1, LLVMValueRef ValueE2) {
  switch (Op) {
    case OrOp:   return LLVMBuildOr (Builder, ValueE1, ValueE2, ""); 
    case AndOp:  return LLVMBuildAnd(Builder, ValueE1, ValueE2, ""); 
    case SumOp:  return LLVMBuildAdd(Builder, ValueE1, ValueE2, ""); 
    case SubOp:  return LLVMBuildSub(Builder, ValueE1, ValueE2, ""); 
    case MultOp: return LLVMBuildMul(Builder, ValueE1, ValueE2, ""); 
    case DivOp:  return LLVMBuildSDiv(Builder, ValueE1, ValueE2, ""); 
    case LtOp:   return LLVMBuildICmp(Builder, LLVMIntSLT, ValueE1, ValueE2, ""); 
    case LeOp:   return LLVMBuildICmp(Builder, LLVMIntSLE, ValueE1, ValueE2, ""); 
    case GtOp:   return LLVMBuildICmp(Builder, LLVMIntSGT, ValueE1, ValueE2, ""); 
    case GeOp:   return LLVMBuildICmp(Builder, LLVMIntSGE, ValueE1, ValueE2, ""); 
    case EqOp:   return LLVMBuildICmp(Builder, LLVMIntEQ,  ValueE1, ValueE2, ""); 
    case DiffOp: return LLVMBuildICmp(Builder, LLVMIntNE,  ValueE1, ValueE2, ""); 
    default:     return NULL;
  }
}

static LLVMValueRef
translateFloatBinOp(NodeKind Op, LLVMValueRef ValueE1, LLVMValueRef ValueE2) {
  switch (Op) {
    case SumOp:  return LLVMBuildFAdd(Builder, ValueE1, ValueE2, ""); 
    case SubOp:  return LLVMBuildFSub(Builder, ValueE1, ValueE2, ""); 
    case MultOp: return LLVMBuildFMul(Builder, ValueE1, ValueE2, ""); 
    case DivOp:  return LLVMBuildFDiv(Builder, ValueE1, ValueE2, ""); 
    case LtOp:   return LLVMBuildFCmp(Builder, LLVMRealOLT, ValueE1, ValueE2, ""); 
    case LeOp:   return LLVMBuildFCmp(Builder, LLVMRealOLE, ValueE1, ValueE2, ""); 
    case GtOp:   return LLVMBuildFCmp(Builder, LLVMRealOGT, ValueE1, ValueE2, ""); 
    case GeOp:   return LLVMBuildFCmp(Builder, LLVMRealOGE, ValueE1, ValueE2, ""); 
    case EqOp:   return LLVMBuildFCmp(Builder, LLVMRealOEQ, ValueE1, ValueE2, ""); 
    case DiffOp: return LLVMBuildFCmp(Builder, LLVMRealONE, ValueE1, ValueE2, ""); 
    default:     return NULL;
  }
}

static LLVMValueRef
translateStringBinOp(NodeKind Op, LLVMValueRef StrCmpFn, LLVMValueRef ValueE1, LLVMValueRef ValueE2) {
  LLVMValueRef CmpArgs[]  = { ValueE1, ValueE2 },
               CallStrCmp = LLVMBuildCall(Builder, StrCmpFn, CmpArgs, 2, ""),
               ZeroConst  = LLVMConstInt(LLVMInt32Type(), 0, 1);
  switch (Op) {
    case LtOp:   return LLVMBuildICmp(Builder, LLVMIntSLT, CallStrCmp, ZeroConst, ""); 
    case LeOp:   return LLVMBuildICmp(Builder, LLVMIntSLE, CallStrCmp, ZeroConst, "");
    case GtOp:   return LLVMBuildICmp(Builder, LLVMIntSGT, CallStrCmp, ZeroConst, "");
    case GeOp:   return LLVMBuildICmp(Builder, LLVMIntSGE, CallStrCmp, ZeroConst, "");
    case EqOp:   return LLVMBuildICmp(Builder, LLVMIntEQ,  CallStrCmp, ZeroConst, "");
    case DiffOp: return LLVMBuildICmp(Builder, LLVMIntNE,  CallStrCmp, ZeroConst, "");
    default:     return NULL;
  }
}

static LLVMValueRef
translateStructBinOp(NodeKind Op, LLVMValueRef ValueE1, LLVMValueRef ValueE2) {
  switch (Op) {
    case EqOp:   return LLVMBuildICmp(Builder, LLVMIntEQ, ValueE1, ValueE2, "");
    case DiffOp: return LLVMBuildICmp(Builder, LLVMIntNE, ValueE1, ValueE2, "");
    default:     return NULL;
  }
}

static LLVMValueRef
translateBinOp(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  ASTNode *NodeE1 = (ASTNode*) ptrVectorGet(&(Node->Child), 0),
          *NodeE2 = (ASTNode*) ptrVectorGet(&(Node->Child), 0);

  LLVMValueRef ValueE1 = translateExpr(TyTable, ValTable, NodeE1),
               ValueE2 = translateExpr(TyTable, ValTable, NodeE2);

  char Buf[] = "strcmp";
  LLVMValueRef StrCmpFn = symTableFindGlobal(ValTable, Buf);
  
  switch (LLVMGetTypeKind(LLVMTypeOf(ValueE1))) {
    case LLVMIntegerTypeKind: return translateIntBinOp   (Node->Kind, ValueE1, ValueE2);
    case LLVMFloatTypeKind:   return translateFloatBinOp (Node->Kind, ValueE1, ValueE2);
    case LLVMPointerTypeKind: 
      {
        if (LLVMGetTypeKind(LLVMGetElementType(LLVMTypeOf(ValueE1))) == LLVMStructTypeKind)
          return translateStructBinOp(Node->Kind, ValueE1, ValueE2);
        return translateStringBinOp(Node->Kind, StrCmpFn, ValueE1, ValueE2);
      }
    default: return NULL;
  }
}

static LLVMValueRef
translateNegOp(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  ASTNode *Expr = (ASTNode*) ptrVectorGet(&(Node->Child), 0);

  LLVMValueRef ExprVal = translateExpr(TyTable, ValTable, Expr);
  return LLVMBuildNeg(Builder, ExprVal, "");
}

static LLVMValueRef
translateLetExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  SymbolTable *TyTable_  = symTableFindChild(TyTable, Node),
              *ValTable_ = symTableFindChild(ValTable, Node);

  translateDecl(TyTable_, ValTable_, ptrVectorGet(&(Node->Child), 0));
  return translateExpr(TyTable_, ValTable_, ptrVectorGet(&(Node->Child), 1));
}

static void createBasicBlockName(char *Dst, const char *Prefix) {
  sprintf(Dst, "%s%d", Prefix, BBCount++);
}

static LLVMValueRef
translateIfThenExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  char BBName[NAME_MAX];
  ASTNode *CondNode = (ASTNode*) ptrVectorGet(&(Node->Child), 0),
          *ThenNode = (ASTNode*) ptrVectorGet(&(Node->Child), 1),
          *ElseNode = (ASTNode*) ptrVectorGet(&(Node->Child), 2);

  LLVMBasicBlockRef ThisBB = LLVMGetInsertBlock(Builder);
  LLVMValueRef      ThisFn = LLVMGetBasicBlockParent(ThisBB);

  // Creating the BasicBlocks that will be used.
  LLVMBasicBlockRef TrueBB, FalseBB, EndBB;

  createBasicBlockName(BBName, "if.then.");
  TrueBB = LLVMAppendBasicBlock(ThisFn, BBName);
  createBasicBlockName(BBName, "if.end.");
  EndBB = LLVMAppendBasicBlock(ThisFn, BBName);

  if (ElseNode) {
    createBasicBlockName(BBName, "if.else.");
    FalseBB = LLVMAppendBasicBlock(ThisFn, BBName);
  } else FalseBB = EndBB;

  // Creating the conditional branch.
  LLVMValueRef CondValue = translateExpr(TyTable, ValTable, CondNode);
  LLVMBuildCondBr(Builder, CondValue, TrueBB, FalseBB);

  // Filling the BasicBlocks.
  LLVMValueRef TrueValue, FalseValue;

  LLVMPositionBuilderAtEnd(Builder, TrueBB);
  TrueValue = translateExpr(TyTable, ValTable, ThenNode);
  LLVMBuildBr(Builder, EndBB);

  if (ElseNode) {
    LLVMPositionBuilderAtEnd(Builder, FalseBB);
    FalseValue = translateExpr(TyTable, ValTable, ElseNode);
    LLVMBuildBr(Builder, EndBB);
  }

  LLVMPositionBuilderAtEnd(Builder, EndBB);
  if (ElseNode) {
    LLVMValueRef PhiNode = LLVMBuildPhi
      (Builder, LLVMGetElementType(LLVMTypeOf(TrueValue)), "");
    // Adding incoming to phi-node.
    LLVMValueRef      Values[] = { TrueValue, FalseValue };
    LLVMBasicBlockRef Blocks[] = { TrueBB,    FalseBB };
    LLVMAddIncoming(PhiNode, Values, Blocks, 2);
    return PhiNode;
  }
  return NULL; 
}

static LLVMValueRef
translateFunCallExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);

  ASTNode *ExprNode   = (ASTNode*) ptrVectorGet(V, 0),
          *ParamsNode = (ASTNode*) ptrVectorGet(V, 1);

  unsigned Count;
  LLVMValueRef ParamsVal[ParamsNode->Child.Size];
  for (Count = 0; Count < ParamsNode->Child.Size; ++Count) {
    ParamsVal[Count] = translateExpr(TyTable, ValTable, Node);
  }

  LLVMTypeRef FunctionType = (LLVMTypeRef) getAliasValue(ValTable, Node->Value, &toFunctionName, &symTableFindGlobal);
  LLVMValueRef Closure = translateExpr(TyTable, ValTable, ExprNode);

  LLVMValueRef DtIdx[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 1),
    LLVMConstInt(LLVMInt32Type(), 0, 1)
  };
  LLVMValueRef DataRef = LLVMBuildInBoundsGEP(Builder, Closure, DtIdx, 2, "");

  LLVMValueRef FnIdx[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 1),
    LLVMConstInt(LLVMInt32Type(), 1, 1)
  };
  LLVMValueRef FnPtr = LLVMBuildInBoundsGEP(Builder, Closure, FnIdx, 2, "");
  LLVMValueRef Function = LLVMBuildBitCast(Builder, FnPtr, FunctionType, "");

  insertNewRA(LLVMBuildLoad(Builder, DataRef, ""));
  LLVMValueRef CallValue = LLVMBuildCall(Builder, Function, ParamsVal, Count, "");
  finalizeExecutionRA();
  return CallValue;
}

static LLVMValueRef
translateArrayExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);

  ASTNode *SizeNode = (ASTNode*) ptrVectorGet(V, 0),
          *InitNode = (ASTNode*) ptrVectorGet(V, 1);
  Type    *ThisType = (Type*) symTableFind(TyTable, Node->Value);

  LLVMTypeRef ArrayType = getLLVMTypeFromType(TyTable, ThisType);

  LLVMValueRef SizeVal = translateExpr(TyTable, ValTable, SizeNode),
               InitVal = translateExpr(TyTable, ValTable, InitNode);

  LLVMValueRef ArrayVal = LLVMBuildArrayMalloc(Builder, ArrayType, SizeVal, "");

  // This BasicBlock and ThisFunction
  LLVMBasicBlockRef ThisBB = LLVMGetInsertBlock(Builder);
  LLVMValueRef      ThisFn = LLVMGetBasicBlockParent(ThisBB);

  LLVMValueRef Counter = LLVMBuildAlloca(Builder, LLVMInt32Type(), "");
  LLVMBuildStore(Builder, LLVMConstInt(LLVMInt32Type(), 0, 1), Counter);

  LLVMTargetDataRef DataRef = LLVMCreateTargetData(LLVMGetDataLayout(Module));
  unsigned long long Size = LLVMStoreSizeOfType(DataRef, ArrayType);

  char BBName[NAME_MAX];
  LLVMBasicBlockRef InitBB, MidBB, EndBB;

  createBasicBlockName(BBName, "for.init");
  InitBB = LLVMAppendBasicBlock(ThisFn, BBName);
  createBasicBlockName(BBName, "for.end");
  EndBB = LLVMAppendBasicBlock(ThisFn, BBName);
  createBasicBlockName(BBName, "for.mid");
  MidBB = LLVMAppendBasicBlock(ThisFn, BBName);

  LLVMBuildBr(Builder, InitBB);

  LLVMPositionBuilderAtEnd(Builder, InitBB);
  LLVMValueRef CurrentCounter = LLVMBuildLoad(Builder, Counter, "");
  LLVMValueRef Comparation    = LLVMBuildICmp(Builder, LLVMIntSLT, CurrentCounter, SizeVal, "");
  LLVMBuildCondBr(Builder, Comparation, MidBB, EndBB);

  LLVMPositionBuilderAtEnd(Builder, MidBB);
  CurrentCounter = LLVMBuildLoad(Builder, Counter, "");
  LLVMValueRef TheValue = LLVMBuildLoad(Builder, InitVal, ""); 
  LLVMValueRef ElemIdx[] = { LLVMConstInt(LLVMInt32Type(), 0, 1), CurrentCounter };
  LLVMValueRef Elem = LLVMBuildInBoundsGEP(Builder, ArrayVal, ElemIdx, 2, "");
  copyMemory(Elem, TheValue, Size); 
  LLVMBuildBr(Builder, InitBB);

  LLVMPositionBuilderAtEnd(Builder, EndBB);
  return ArrayVal;
}

static LLVMValueRef
translateRecordExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);

  ASTNode *FieldNode = (ASTNode*) ptrVectorGet(V, 0);
  Type    *ThisType  = (Type*) symTableFind(TyTable, Node->Value);

  LLVMTypeRef RecordType = getLLVMTypeFromType(TyTable, ThisType);

  LLVMValueRef RecordVal = LLVMBuildMalloc(Builder, RecordType, "");
  unsigned FieldNumber   = LLVMCountStructElementTypes(RecordType),
           I;
  for (I = 0; I < FieldNumber; ++I) {
    LLVMTargetDataRef DataRef = LLVMCreateTargetData(LLVMGetDataLayout(Module));
    unsigned long long Size = LLVMStoreSizeOfType(DataRef, LLVMStructGetTypeAtIndex(RecordType, I));

    LLVMValueRef ElemIdx[] = {
      LLVMConstInt(LLVMInt32Type(), 0, 1),   
      LLVMConstInt(LLVMInt32Type(), I, 1)   
    };
    LLVMValueRef ElemI = LLVMBuildInBoundsGEP(Builder, RecordVal, ElemIdx, 2, "");
    LLVMValueRef FieldExpr = translateExpr(TyTable, ValTable, ptrVectorGet(&(FieldNode->Child), I));
    copyMemory(ElemI, FieldExpr, Size);
  }
  return RecordVal; 
}

static LLVMValueRef
translateNilExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  if (!Node->Value) exit(1);
  Type       *RecordType = getRealType(TyTable, symTableFind(TyTable, Node->Value));
  LLVMTypeRef Record     = LLVMGetTypeByName(Module, RecordType->Val);
  return LLVMConstPointerNull(Record);
}

LLVMValueRef translateExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  if (!Node) return NULL;

  switch (Node->Kind) {
    case IntLit:    return translateIntLit   (Node);
    case FloatLit:  return translateFloatLit (Node);
    case StringLit: return translateStringLit(Node);

    case IdLval:        return translateIdLval       (TyTable, ValTable, Node);
    case RecAccessLval: return translateRecAccessLval(TyTable, ValTable, Node);
    case ArrAccessLval: return translateArrAccessLval(TyTable, ValTable, Node);

    case AndOp:
    case OrOp:
    case SumOp:
    case SubOp:
    case MultOp:
    case DivOp:
    case LtOp:
    case LeOp:
    case GtOp:
    case GeOp:
    case EqOp:
    case DiffOp: return translateBinOp(TyTable, ValTable, Node);
    case NegOp:  return translateNegOp(TyTable, ValTable, Node);

    case LetExpr:     return translateLetExpr    (TyTable, ValTable, Node);
    case IfStmtExpr:  return translateIfThenExpr (TyTable, ValTable, Node);
    case FunCallExpr: return translateFunCallExpr(TyTable, ValTable, Node);
    case ArrayExpr:   return translateArrayExpr  (TyTable, ValTable, Node); 
    case RecordExpr:  return translateRecordExpr (TyTable, ValTable, Node);

    case NilExpr: return translateNilExpr(TyTable, ValTable, Node);

    default:      return NULL;
  }
}
