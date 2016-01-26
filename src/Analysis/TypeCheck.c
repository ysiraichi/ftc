#include "ftc/Analysis/Types.h"
#include "ftc/Analysis/SymbolTable.h"

Type *checkExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  Type *T;
  PtrVector *V = &(Node->Child);
  switch (Node->Kind) {
    case IntLit:
      return createSimpleType(IntTy);
    case FloatLit:
      return createSimpleType(FloatTy);
    case StringLit:
      return createSimpleType(StringTy);
    case IdLval:
      return hashFind(ValTable, Node->Value);
    case RecAccessLval:
      T = checkExpr(ptrVectorGet(V, 0));

    case ArrAccessLval:
    case SumOp:
    case SubOp:
    case MultOp:
    case DivOp:
    case EqOp:
    case DiffOp:
    case LtOp:
    case LeOp:
    case GtOp:
    case GeOp:
    case AndOp:
    case OrOp:
    case NegOp:
    case LetExpr:
    case IfStmtExpr:
    case FunCallExpr:
    case ArgExpr:
    case ArrayExpr:
    case RecordExpr:
    case FieldExpr:
  }
}

