#include "ftc/Analysis/TypeCheck.h"
#include "ftc/Analysis/Types.h"
#include "ftc/Analysis/SymbolTable.h"

static Type*
checkIdLval(SymbolTable *ValTable, ASTNode *Node) {

  if (symTableExists(ValTable, Node->Value)) {
    insertIfEscaped(ValTable, Node->Value);
    return symTableFind(ValTable, Node->Value);
  } else semError(1, Node, "Identifier '%s' not declared.", Node->Value);
  // unreachable
  return NULL;
}

static Type*
checkRecAccessLval(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *V = &(Node->Child);

  Type *ExprType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)),
       *Resolved = resolveType(TyTable, ExprType); 
  if (Resolved->Kind != IdTy) semError(1, Node, "Variable type is not a record.");

  Type *RealType = (Type*) symTableFind(TyTable, Resolved->Val);
  if (RealType->Kind == RecordTy) {
    Hash *RecordScope = (Hash*) RealType->Val;
    if (hashExists(RecordScope, Node->Value)) 
      return hashFind(RecordScope, Node->Value);
    else semError(1, Node, "Type '%s' has no member '%s'.", ExprType->Val, Node->Value);
  } else semError(1, Node, "Variable type is not a record.");
  // unreachable
  return NULL;
}

static Type*
checkArrAccessLval(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *V = &(Node->Child);

  Type *ExprType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)),
       *Resolved = resolveType(TyTable, ExprType);
  if (Resolved->Kind != IdTy) semError(1, Node, "Variable type is not a record.");

  Type *RealType = (Type*) symTableFind(TyTable, Resolved->Val);
  if (RealType->Kind == ArrayTy) {
    Type *IdxType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
    if (IdxType->Kind == IntTy) {
      return (Type*) RealType->Val;
    } else semError(1, Node, "Index is not an integer number.");
  } else semError(1, Node, "Variable type is not an array.");
  // unreachable
  return NULL;
}

static Type*
checkFloatBinOp(ASTNode *Node, Type *Ty) {

  switch (Node->Kind) {
    case AndOp:
    case OrOp: semError(1, Node, "Boolean operation permited only on Int type.");

    case LtOp:
    case LeOp:
    case GtOp:
    case GeOp: 
    case EqOp:
    case DiffOp: return createType(IntTy, NULL);

    default: return Ty;
  }
  // unreachable
  return NULL;
}

static Type*
checkStringBinOp(ASTNode *Node) {

  switch (Node->Kind) {
    case SumOp:
    case SubOp:
    case MultOp:
    case DivOp: semError(1, Node, "Arithmetic operation permited only on same type, Float or Int operands.");

    case AndOp:
    case OrOp: semError(1, Node, "Boolean operation permited only on Int type.");

    default: return createType(IntTy, NULL);
  }
  // unreachable
  return NULL;
}

static Type*
checkStructBinOp(ASTNode *Node) {

  switch (Node->Kind) {
    case EqOp:
    case DiffOp: return createType(IntTy, NULL);

    default: semError(1, Node, "Operation not permited with composite type.");
  }
  // unreachable
  return NULL;
}

static Type*
checkBinOp(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *V = &(Node->Child);

  Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)), 
       *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));

  if (typeEqual(TyTable, E1, E2)) {
    switch (E1->Kind) {
      case IntTy:    return E1;
      case FloatTy:  return checkFloatBinOp (Node, E1);
      case StringTy: return checkStringBinOp(Node);
      case IdTy:     return checkStructBinOp(Node);

      default: semError(1, Node, "Invalid operators type in binary operation.");
    }
  } else semError(1, Node, "Operators must be of the same type.");
  // unreachable
  return NULL;
}

static Type*
checkNegOp(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *V = &(Node->Child);

  Type *ExprType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));

  if (ExprType->Kind == IntTy || ExprType->Kind == FloatTy) return ExprType;
  else semError(1, Node, "Unary minus operation permited only on Float or Int operand.");
  // unreachable
  return NULL;
}

static Type*
checkLetExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);
  SymbolTable *TyTable_  = createSymbolTable(Node, TyTable),
              *ValTable_ = createSymbolTable(Node, ValTable);

  checkDecl(TyTable_, ValTable_, ptrVectorGet(V, 0));
  return checkExpr(TyTable_, ValTable_, ptrVectorGet(V, 1));
}

static Type*
checkIfThenExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *V = &(Node->Child);
  Type *ExprType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));

  if (ExprType->Kind == IntTy) {
    Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1)), 
         *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 2));

    if (E2 && typeEqual(TyTable, E1, E2)) return E1;
    else if (E2) semError(1, Node, 
        "Expressions after 'THEN' and 'ELSE' must be of the same type.");
    else if (E1 && E1->Kind == AnswerTy) return E1;
    else semError(1, Node, "Expression not returning 'answer' after 'THEN'.");

  } else semError(1, Node, "Condition does not result in integer.");
  // unreachable
  return NULL;
}

static Type*
checkFunCallExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *V = &(Node->Child);
  Type *FnType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));

  FnType = resolveType(TyTable, FnType);
  if (FnType->Kind == FunTy) {
    Type *ArgType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
    if (typeEqual(TyTable, ((Type**) FnType->Val)[0], ArgType))
      return (Type*) ((Type**) FnType->Val)[1];
    else semError(1, Node, "Arguments do not match the call.");

  } else semError(1, Node, "Expression does not result in function to call.");
  // unreachable
  return NULL;
}

static Type*
checkArgExprList(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *Args;
  PtrVector *V = &(Node->Child);
  PtrVectorIterator I = beginPtrVector(V), 
                    E = endPtrVector(V);
  Args = createPtrVector();
  for (; I != E; ++I) 
    ptrVectorAppend(Args, checkExpr(TyTable, ValTable, *I));
  return createType(SeqTy, Args); 
}

static Type*
checkArrayExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *V = &(Node->Child);
  Type *NodeType = resolveType(TyTable, createType(IdTy, Node->Value)),
       *ThisType = (Type*) symTableFind(TyTable, NodeType->Val);

  if (!ThisType) semError(1, Node, "Undefined type '%s'.", Node->Value);

  if (ThisType->Kind == ArrayTy) {
    Type *SizeType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));

    if (SizeType && SizeType->Kind == IntTy) {
      Type *InitType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1)); 

      if (InitType && typeEqual(TyTable, ThisType->Val, InitType)) {
        return createType(IdTy, Node->Value);
      } else semError(1, Node, "Init expression is not the same type as the array.");

    } else semError(1, Node, "Size expression is not an integer type.");
  } else semError(1, Node, "Type '%s' is not an array type.", Node->Value);
  // unreachable
  return NULL;
}

static Type*
checkRecordExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *V = &(Node->Child);
  PtrVectorIterator B, E;
  Type *NodeType = resolveType(TyTable, createType(IdTy, Node->Value)),
       *ThisType = (Type*) symTableFind(TyTable, NodeType->Val);

  if (!ThisType) semError(1, Node, "Undefined type '%s'.", Node->Value);

  if (ThisType->Kind == RecordTy) {
    int Success = 1;
    Type *FieldsTy = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
    Hash *RecScope = (Hash*) ThisType->Val,
         *Record   = (Hash*) FieldsTy->Val;

    if (RecScope->Pairs.Size == Record->Pairs.Size) {
      for (B = beginHash(RecScope), E = endHash(RecScope); B != E; ++B) {
        Pair *P = (Pair*) *B;
        Success = Success && hashExists(Record, P->Key) && 
          typeEqual(TyTable, P->Value, hashFind(Record, P->Key));
        if (!Success) break;
      }

      if (Success) return createType(IdTy, Node->Value);
      else semError(1, Node, "Record '%s' did not match call.", Node->Value);

    } else semError(1, Node, "Record '%s' did not match call.", Node->Value);
  } else semError(1, Node, "Type '%s' is not a record.", Node->Value);
  // unreachable
  return NULL;
}

static Type*
checkFieldExprList(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {

  PtrVector *V = &(Node->Child);
  PtrVectorIterator I = beginPtrVector(V),
                    E = endPtrVector(V);

  Hash *Fields = createHash();
  for (; I != E; ++I) {
    ASTNode *N      = (ASTNode*) *I,
            *TyNode = (ASTNode*) ptrVectorGet(&(N->Child), 0);
    hashInsert(Fields, N->Value, checkExpr(TyTable, ValTable, TyNode)); 
  }
  return createType(RecordTy, Fields);
}

/* <function> */
Type *checkExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  if (!Node) return NULL;

  switch (Node->Kind) {
    case IntLit:    return createType(IntTy, NULL);
    case FloatLit:  return createType(FloatTy, NULL);
    case StringLit: return createType(StringTy, NULL);

    case IdLval:        return checkIdLval       (ValTable, Node);
    case RecAccessLval: return checkRecAccessLval(TyTable, ValTable, Node);
    case ArrAccessLval: return checkArrAccessLval(TyTable, ValTable, Node);

    case SumOp:
    case SubOp:
    case MultOp:
    case DivOp:
    case AndOp:
    case OrOp:
    case LtOp:
    case LeOp:
    case GtOp:
    case GeOp:
    case EqOp:
    case DiffOp: return checkBinOp(TyTable, ValTable, Node);
    case NegOp:  return checkNegOp(TyTable, ValTable, Node);

    case LetExpr:       return checkLetExpr(TyTable, ValTable, Node);
    case IfStmtExpr:    return checkIfThenExpr(TyTable, ValTable, Node);
    case FunCallExpr:   return checkFunCallExpr(TyTable, ValTable, Node);
    case ArgExprList:   return checkArgExprList(TyTable, ValTable, Node);
    case ArrayExpr:     return checkArrayExpr(TyTable, ValTable, Node);
    case RecordExpr:    return checkRecordExpr(TyTable, ValTable, Node);
    case FieldExprList: return checkFieldExprList(TyTable, ValTable, Node);

    case NilExpr: return createType(NilTy, NULL);

    default: return NULL;
  }
}

