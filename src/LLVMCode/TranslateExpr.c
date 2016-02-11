#include "ftc/LLVMCode/Translate.h"

#include <stdio.h>
#include <string.h>

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

static LLVMValueRef 
translateIntLit(ASTNode *Node) {
  printf("IntLit\n");
  LLVMValueRef Container = LLVMBuildAlloca(Builder, LLVMInt32Type(), "");
  LLVMBuildStore(Builder, getSConstInt(*((int*)Node->Value)), Container);
  return Container;
}

static LLVMValueRef 
translateFloatLit(ASTNode *Node) {
  printf("FloatLit\n");
  LLVMValueRef Container = LLVMBuildAlloca(Builder, LLVMFloatType(), "");
  LLVMBuildStore(Builder, LLVMConstReal(LLVMFloatType(), *((float*)Node->Value)), Container);
  return Container;
}

static LLVMValueRef 
translateStringLit(ASTNode *Node) {
  printf("StringLit\n");
  int Length = strlen(Node->Value);

  LLVMValueRef GlobVar = LLVMAddGlobal(Module, LLVMArrayType(LLVMInt8Type(), Length+1), "global.var");
  LLVMSetInitializer(GlobVar, LLVMConstString(Node->Value, Length, 0));

  LLVMValueRef LocalVar = LLVMBuildAlloca(Builder, LLVMArrayType(LLVMInt8Type(), Length+1), "local.string.");
  LLVMValueRef LocalI8  = LLVMBuildBitCast(Builder, LocalVar, LLVMPointerType(LLVMInt8Type(), 0), "");
  copyMemory(LocalI8, GlobVar, getSConstInt(Length+1));
  return wrapValue(LocalI8);
}

static LLVMValueRef 
translateIdLval(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("IdLval\n");
  Type *IdType = (Type*) symTableFind(ValTable, Node->Value);
  LLVMValueRef IdValue;

  if (IdType->EscapedLevel > 0) {
    LLVMTypeRef LLVMType = getLLVMTypeFromType(TyTable, IdType);
    LLVMValueRef EVPtr   = getEscapedVar(ValTable, Node->Value, Node->EscapedLevel);
    LLVMValueRef EVLoad  = LLVMBuildLoad(Builder, EVPtr, "");

    IdValue = LLVMBuildBitCast(Builder, EVLoad, LLVMPointerType(LLVMType, 0), "");
  } else {
    IdValue = resolveAliasId(ValTable, Node->Value, &toValName, &symTableFindLocal);
  }

  return IdValue;
}

static LLVMValueRef 
translateRecAccessLval(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("RecAccessLval\n");
  ASTNode *Lval = (ASTNode*) ptrVectorGet(&(Node->Child), 0);

  LLVMValueRef Record     = translateExpr(TyTable, ValTable, Lval);
  LLVMTypeRef  RecTypeRef = LLVMGetElementType(LLVMTypeOf(Record));

  if (LLVMGetTypeKind(RecTypeRef) == LLVMPointerTypeKind)
    RecTypeRef = LLVMGetElementType(RecTypeRef);

  /* Get struct's name. */
  const char *Buf = LLVMGetStructName(RecTypeRef); 
  char TypeName[NAME_MAX];
  strcpy(TypeName, Buf);

  /* Get struct's field. */
  Type      *RecType = (Type*) symTableFind(TyTable, TypeName);
  Hash      *Fields  = (Hash*) RecType->Val;
  PtrVector *V       = &(Fields->Pairs);

  /* Get the right struct's field. */
  unsigned Count = 0;
  for (Count = 0; Count < V->Size; ++Count) {
    Pair *P = (Pair*) ptrVectorGet(V, Count);
    if (!strcmp(P->first, Node->Value)) break;
  }

  LLVMValueRef Idx[] = { getSConstInt(0), getSConstInt(Count) };
  return LLVMBuildInBoundsGEP(Builder, Record, Idx, 2, "");
}

static LLVMValueRef 
translateArrAccessLval(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("ArrAccessLval\n");
  ASTNode *Lval  = (ASTNode*) ptrVectorGet(&(Node->Child), 0),
          *Index = (ASTNode*) ptrVectorGet(&(Node->Child), 1);

  LLVMValueRef Array  = translateExpr(TyTable, ValTable, Lval);
  LLVMValueRef IndVal = translateExpr(TyTable, ValTable, Index);
  LLVMValueRef Idx[] = { getSConstInt(0), IndVal };
  return LLVMBuildInBoundsGEP(Builder, Array, Idx, 2, ""); 
}

static LLVMValueRef
translateIntBinOp(NodeKind Op, LLVMValueRef ValueE1, LLVMValueRef ValueE2) {
  printf("IntBinOp\n");
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
  printf("FloatBinOp\n");
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
  printf("StringBinOp\n");
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
  printf("StructBinOp\n");
  switch (Op) {
    case EqOp:   return LLVMBuildICmp(Builder, LLVMIntEQ, ValueE1, ValueE2, "");
    case DiffOp: return LLVMBuildICmp(Builder, LLVMIntNE, ValueE1, ValueE2, "");
    default:     return NULL;
  }
}

static LLVMValueRef
translateBinOp(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("BinOp\n");
  ASTNode *NodeE1 = (ASTNode*) ptrVectorGet(&(Node->Child), 0),
          *NodeE2 = (ASTNode*) ptrVectorGet(&(Node->Child), 1);

  LLVMValueRef ValueE1Ptr = translateExpr(TyTable, ValTable, NodeE1),
               ValueE2Ptr = translateExpr(TyTable, ValTable, NodeE2);

  LLVMValueRef ValueE1 = LLVMBuildLoad(Builder, ValueE1Ptr, "binop.ld.e1."),
               ValueE2 = LLVMBuildLoad(Builder, ValueE2Ptr, "binop.ld.e2.");

  LLVMValueRef StrCmpFn  = symTableFindGlobal(ValTable, "strcmp");
  LLVMValueRef ResultVal = NULL;
  switch (LLVMGetTypeKind(LLVMTypeOf(ValueE1))) {
    case LLVMIntegerTypeKind: ResultVal = translateIntBinOp   (Node->Kind, ValueE1, ValueE2); break;
    case LLVMFloatTypeKind:   ResultVal = translateFloatBinOp (Node->Kind, ValueE1, ValueE2); break;
    case LLVMStructTypeKind:  ResultVal = translateStructBinOp(Node->Kind, ValueE1, ValueE2); break;
    case LLVMPointerTypeKind: ResultVal = translateStringBinOp(Node->Kind, StrCmpFn, ValueE1, ValueE2); break;

    default: return NULL;
  }

  switch (LLVMGetTypeKind(LLVMTypeOf(ResultVal))) {
    case LLVMIntegerTypeKind: ResultVal = LLVMBuildZExt(Builder, ResultVal, LLVMInt32Type(), ""); break;
    default: break;
  }
  return wrapValue(ResultVal);
}

static LLVMValueRef
translateNegOp(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("NegOp\n");
  ASTNode *Expr = (ASTNode*) ptrVectorGet(&(Node->Child), 0);

  LLVMValueRef ExprPtr  = translateExpr(TyTable, ValTable, Expr);
  LLVMValueRef ExprLoad = LLVMBuildLoad(Builder, ExprPtr, "");
  LLVMBuildNeg (Builder, ExprLoad, "");

  return ExprPtr;
}

static LLVMValueRef
translateLetExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("LetExpr\n");
  SymbolTable *TyTable_  = symTableFindChild(TyTable, Node),
              *ValTable_ = symTableFindChild(ValTable, Node);

  translateDecl(TyTable_, ValTable_, ptrVectorGet(&(Node->Child), 0));
  return translateExpr(TyTable_, ValTable_, ptrVectorGet(&(Node->Child), 1));
}

static LLVMValueRef
translateIfThenExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("IfThenExpr\n");
  ASTNode *CondNode = (ASTNode*) ptrVectorGet(&(Node->Child), 0),
          *ThenNode = (ASTNode*) ptrVectorGet(&(Node->Child), 1),
          *ElseNode = (ASTNode*) ptrVectorGet(&(Node->Child), 2);

  LLVMBasicBlockRef ThisBB = LLVMGetInsertBlock(Builder);
  LLVMValueRef      ThisFn = LLVMGetBasicBlockParent(ThisBB);

  // Creating the BasicBlocks that will be used.
  LLVMBasicBlockRef TrueBB, FalseBB, EndBB;

  TrueBB = LLVMAppendBasicBlock(ThisFn, "if.then");
  EndBB  = LLVMAppendBasicBlock(ThisFn, "if.end");

  if (ElseNode) FalseBB = LLVMAppendBasicBlock(ThisFn, "if.else");
  else FalseBB = EndBB;

  // Creating the conditional branch.
  LLVMValueRef CondValue = translateExpr(TyTable, ValTable, CondNode);
  LLVMValueRef CondLoad  = LLVMBuildLoad(Builder, CondValue, "");
  LLVMValueRef CalcTrueFalse = LLVMBuildICmp(Builder, LLVMIntNE, CondLoad, getSConstInt(0), "");
  LLVMBuildCondBr(Builder, CalcTrueFalse, TrueBB, FalseBB);

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
  if (ElseNode && LLVMGetTypeKind(LLVMTypeOf(TrueValue)) != LLVMVoidTypeKind) {
    LLVMValueRef PhiNode = LLVMBuildPhi(Builder, LLVMTypeOf(TrueValue), "");
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
  printf("FunCallExpr\n");
  PtrVector *V = &(Node->Child);

  ASTNode *ExprNode   = (ASTNode*) ptrVectorGet(V, 0),
          *ParamsNode = (ASTNode*) ptrVectorGet(V, 1);

  LLVMTypeRef ReturnType  = toTransitionType(getLLVMTypeFromType(TyTable, Node->Value)),
              *ParamsType = NULL, FunctionType;
  LLVMValueRef *ParamsValue = NULL;

  unsigned Count = 0;
  if (ParamsNode) {
    ParamsValue = (LLVMValueRef*) malloc(sizeof(LLVMValueRef) * ParamsNode->Child.Size);
    for (Count = 0; Count < ParamsNode->Child.Size; ++Count) {
      LLVMValueRef ExprVal = translateExpr(TyTable, ValTable, ptrVectorGet(&(ParamsNode->Child), Count));

      LLVMTypeRef ExprType = LLVMGetElementType(LLVMTypeOf(ExprVal));
      switch (LLVMGetTypeKind(ExprType)) {
        case LLVMIntegerTypeKind:
        case LLVMFloatTypeKind:
        case LLVMPointerTypeKind: ExprVal = LLVMBuildLoad(Builder, ExprVal, "load.4.call"); break;

        default: break;
      }

      ParamsValue[Count]  = ExprVal;

      if (!ParamsType) 
        ParamsType = (LLVMTypeRef*) malloc(sizeof(LLVMTypeRef) * ParamsNode->Child.Size);
      ParamsType[Count] = LLVMTypeOf(ExprVal);
    }
  }

  FunctionType = LLVMFunctionType(ReturnType, ParamsType, Count, 0);
  FunctionType = LLVMPointerType(FunctionType, 0);

  LLVMValueRef Closure   = translateExpr(TyTable, ValTable, ExprNode);
  LLVMValueRef CallValue = callClosure(FunctionType, Closure, ParamsValue, Count);

  switch (getLLVMValueTypeKind(CallValue)) {
    case LLVMIntegerTypeKind:
    case LLVMFloatTypeKind:
    case LLVMPointerTypeKind:
      {
        if (getLLVMValueTypeKind(CallValue) == LLVMPointerTypeKind &&
            getLLVMElementTypeKind(CallValue) == LLVMStructTypeKind) 
          break;
        LLVMValueRef PtrMem = LLVMBuildAlloca(Builder, LLVMTypeOf(CallValue), "");
        LLVMBuildStore(Builder, CallValue, PtrMem);
        return PtrMem;
      }
  }
  return CallValue;
}

static LLVMValueRef
translateArrayExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("ArrayExpr\n");
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

  LLVMBasicBlockRef InitBB, MidBB, EndBB;

  InitBB = LLVMAppendBasicBlock(ThisFn, "for.init");
  EndBB  = LLVMAppendBasicBlock(ThisFn, "for.end");
  MidBB  = LLVMAppendBasicBlock(ThisFn, "for.mid");

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
  copyMemory(Elem, TheValue, getSConstInt(Size)); 
  LLVMBuildBr(Builder, InitBB);

  LLVMPositionBuilderAtEnd(Builder, EndBB);
  return ArrayVal;
}

static LLVMValueRef
translateRecordExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("RecordExpr\n");
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

    LLVMValueRef ElemIdx[] = { getSConstInt(0), getSConstInt(I) };
    LLVMValueRef ElemI     = LLVMBuildInBoundsGEP(Builder, RecordVal, ElemIdx, 2, "");
    LLVMValueRef FieldPtr  = translateExpr(TyTable, ValTable, ptrVectorGet(&(FieldNode->Child), I));
    LLVMValueRef FieldLoad = LLVMBuildLoad(Builder, FieldPtr, "");
    copyMemory(ElemI, FieldLoad, getSConstInt(Size));
  }
  return RecordVal; 
}

static LLVMValueRef
translateNilExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf("NilExpr\n");
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
