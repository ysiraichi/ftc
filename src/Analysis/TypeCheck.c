#include "ftc/Analysis/TypeCheck.h"
#include "ftc/Analysis/Types.h"
#include "ftc/Analysis/SymbolTable.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static int typeEqual(SymbolTable*, Type*, Type*);
static Type *resolveType(SymbolTable*, Type*);
int checkDecl(SymbolTable*, SymbolTable*, ASTNode*);
Type *checkExpr(SymbolTable*, SymbolTable*, ASTNode*);

/* <function> */
static void semError(int Stop, ASTNode *Node, const char *S, ...) {
  printf("@[%u,%u]: ", Node->Pos[0], Node->Pos[1]);
  va_list Args;
  va_start(Args, S);
  vprintf(S, Args);
  va_end(Args);
  printf("\n");

  if (Stop) exit(1);
}

/* <function> */
static Type *getRealType(SymbolTable *TyTable, Type *Ty) {
  if (!Ty) return NULL;
  switch (Ty->Kind) {
    case IdTy:
      {
        Type *Real = getRealType(TyTable, symTableFind(TyTable, Ty->Val));
        if (Real) return Real;
        return createType(IdTy, Ty->Val);
      }
    case ArrayTy:
    case RecordTy:
      return NULL;
    default:
      return createType(Ty->Kind, Ty->Val);
  }
}

/* <function> */
static Type *resolveType(SymbolTable *TyTable, Type *Ty) {
  if (!Ty) return NULL;
  switch (Ty->Kind) {
    case IdTy:
      return getRealType(TyTable, Ty);
    case FunTy:
      {
        Type **Arr = (Type**) Ty->Val;
        Arr[0] = resolveType(TyTable, Arr[0]);
        Arr[1] = resolveType(TyTable, Arr[1]);
      } break;
    case SeqTy:
      {
        PtrVector *V = (PtrVector*) Ty->Val;
        PtrVectorIterator I = beginPtrVector(V),
                          E = endPtrVector(V);
        for (; I != E; ++I)
          *I = resolveType(TyTable, *I);
      } break;
    default:
      break;
  }
  return Ty;
}

/* <function> */
static int typeEqual(SymbolTable *TyTable, Type *T1, Type *T2) {
  if (T1 == T2) return 1;
  T1 = resolveType(TyTable, T1);
  T2 = resolveType(TyTable, T2);
  if (T1->Kind == NilTy && T2->Kind == IdTy) T2 = symTableFind(TyTable, T2->Val);
  if (T2->Kind == NilTy && T1->Kind == IdTy) T1 = symTableFind(TyTable, T1->Val);
  return compareType(T1, T2);
}

/* <function> */
static Type *getTypeFromASTNode(SymbolTable *TyTable, ASTNode *Node) {
  if (!Node || (Node->Kind < ArgDecl && Node->Kind != ArgDeclList) || 
      Node->Kind > RecordTy) return NULL;

  if (Node->Kind < IntTy || Node->Kind > StrConsumerTy) {
    PtrVector *V = &(Node->Child);
    PtrVectorIterator I = beginPtrVector(V),
                      E = endPtrVector(V);
    switch (Node->Kind) {
      case IdTy:
        if (symTableExists(TyTable, Node->Value))
          return createType(IdTy, Node->Value);
        semError(1, Node, "Undefined type '%s'.", Node->Value);
      case ArrayTy:
        {
          ASTNode *TyNode = (ASTNode*) ptrVectorGet(V, 0);
          Type *CoreTy = createType(TyNode->Kind, TyNode->Value);
          return createType(Node->Kind, CoreTy);
        }
      case FunTy:
        {
          Type *From = getTypeFromASTNode(TyTable, ptrVectorGet(V, 0)),
               *To   = getTypeFromASTNode(TyTable, ptrVectorGet(V, 1));
          return createFnType(From, To);
        }
      case SeqTy:
        {
          PtrVector *TypeSeq = createPtrVector();
          for (; I != E; ++I) 
            ptrVectorAppend(TypeSeq, getTypeFromASTNode(TyTable, *I)); 
          return createType(SeqTy, TypeSeq);
        }
      case ArgDeclList:
        {
          Hash *Record = createHash();
          for (; I != E; ++I) {
            ASTNode *N      = (ASTNode*) *I,
                    *TyNode = (ASTNode*) ptrVectorGet(&(N->Child), 0);
            Type    *Ty     = getTypeFromASTNode(TyTable, TyNode);
            if (!Ty && (TyNode->Kind == IdTy && 
                  !symTableExists(TyTable, TyNode->Value))) 
              semError(1, N, "Undefined type '%s'.", (char*) TyNode->Value);
            hashInsert(Record, N->Value, Ty); 
          }
          return createType(RecordTy, Record);
        }
      default:
        return NULL;
    }
  }
  return createType(Node->Kind, NULL);
}

static Type *getTypeFromFnNode(SymbolTable *TyTable, ASTNode *FnNode) {
  PtrVector *Child  = &(FnNode->Child);
  ASTNode   *Params = (ASTNode*) ptrVectorGet(Child, 0),
            *TyNode = (ASTNode*) ptrVectorGet(Child, 1);

  Type *ParamsTy = NULL, 
       *DeclTy   = getTypeFromASTNode(TyTable, TyNode);
  if (Params) {
    PtrVector *ParamsVec = createPtrVector();
    PtrVectorIterator IPar = beginPtrVector(&(Params->Child)),
                      EPar = endPtrVector(&(Params->Child));
    for (; IPar != EPar; ++IPar) {
      ASTNode *Param  = (ASTNode*) *IPar,
              *TyNode = (ASTNode*) ptrVectorGet(&(Param->Child), 0);
      ptrVectorAppend(ParamsVec, getTypeFromASTNode(TyTable, TyNode));
    }
    ParamsTy = createType(SeqTy, ParamsVec);
  }

  if (!DeclTy) DeclTy = createType(AnswerTy, NULL);

  return createFnType(ParamsTy, DeclTy);
}

/* <function> */
void checkCycle(SymbolTable *TyTable, ASTNode *Start) {
  if (Start->Kind != IdTy) return;
  Hash *H = createHash();
  hashInsert(H, Start->Value, NULL);
  Type *Ptr = symTableFind(TyTable, Start->Value);
  while (1) {
    if (Ptr->Kind == IdTy && hashExists(H, Ptr->Val)) {
      Type *Begin = Ptr;
      do {
        semError(0, Start, "Recursive type '%s' is not a record nor an array.", 
            Ptr->Val);
        Ptr = symTableFind(TyTable, Ptr->Val);
      } while (strcmp((char*)Ptr->Val, (char*)Begin->Val));
      exit(1);
    } else if (Ptr->Kind == IdTy) {
      hashInsert(H, Ptr->Val, NULL);
      Ptr = symTableFind(TyTable, Ptr->Val);
    } else return; 
  }
}

/* <function> */
int checkDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);
  PtrVectorIterator I = beginPtrVector(V),
                    E = endPtrVector(V);
  switch (Node->Kind) {
    case DeclList:
      for (; I != E; ++I) checkDecl(TyTable, ValTable, *I);
      break;
    case TyDeclList:
      {
        Hash *ThisBlock = createHash();
        for (; I != E; ++I) {
          ASTNode *N = (ASTNode*) *I;
          if (hashInsert(ThisBlock, N->Value, NULL)) {
            symTableInsertOrChange(TyTable, N->Value, NULL);
          } else semError(1, N, "Type named '%s' already declared in this block.", N->Value);
        }
        destroyHash(ThisBlock, NULL);

        I = beginPtrVector(V);
        E = endPtrVector(V);
        for (; I != E; ++I)
          checkDecl(TyTable, ValTable, *I);

        I = beginPtrVector(V);
        E = endPtrVector(V);
        for (; I != E; ++I) {
          ASTNode *N = (ASTNode*) *I;
          checkCycle(TyTable, ptrVectorGet(&(N->Child), 0));
          checkDecl(TyTable, ValTable, *I);
        }
      } break;
    case FunDeclList:
      {
        Hash *ThisBlock = createHash();
        for (; I != E; ++I) {
          ASTNode *N      = (ASTNode*) *I;
          Type    *FnType = getTypeFromFnNode(TyTable, N);
          if (hashInsert(ThisBlock, N->Value, NULL)) {
            symTableInsertOrChange(ValTable, N->Value, FnType);
          } else semError(1, N, "Function named '%s' already declared in this block.", N->Value);
        }
        destroyHash(ThisBlock, NULL);

        for (I = beginPtrVector(V), E = endPtrVector(V); I != E; ++I)
          checkDecl(TyTable, ValTable, *I);

      } break;
    case VarDecl:
      {
        ASTNode *TyNode = (ASTNode*) ptrVectorGet(V, 0),
                *Expr   = (ASTNode*) ptrVectorGet(V, 1);
        Type *ExprType  = checkExpr(TyTable, ValTable, Expr),
             *DeclType;

        if (ExprType->Kind == NilTy && !TyNode) semError(1, Node, 
            "Initializing var '%s', which is not a record, with 'nil'.", Node->Value);

        if (TyNode) DeclType = getTypeFromASTNode(TyTable, TyNode);
        else DeclType = ExprType;

        if (typeEqual(TyTable, ExprType, DeclType))
          symTableInsertOrChange(ValTable, Node->Value, DeclType); 
        else semError(1, Node, "Incompatible types of variable '%s'.", Node->Value);
      } break;
    case TyDecl:
      {
        ASTNode *TyNode = (ASTNode*) ptrVectorGet(V, 0);
        Type    *RTy    = getTypeFromASTNode(TyTable, TyNode);

        if (TyNode->Kind == IdTy && !symTableExists(TyTable, TyNode->Value)) 
          semError(1, Node, "Undefined type '%s'.", Node->Value);

        symTableInsertOrChange(TyTable, Node->Value, RTy);
      } break;
    case FunDecl: 
      {
        ASTNode *Params = (ASTNode*) ptrVectorGet(V, 0),
                *TyNode = (ASTNode*) ptrVectorGet(V, 1),
                *Expr   = (ASTNode*) ptrVectorGet(V, 2);
        SymbolTable *TyTable_  = createSymbolTable(Node, TyTable),
                    *ValTable_ = createSymbolTable(Node, ValTable);

        if (Params) {
          PtrVectorIterator IPar = beginPtrVector(&(Params->Child)),
                            EPar = endPtrVector(&(Params->Child));
          for (; IPar != EPar; ++IPar) {
            ASTNode *Param  = (ASTNode*) *IPar,
                    *TyNode = (ASTNode*) ptrVectorGet(&(Param->Child), 0);
            Type *ParamType = getTypeFromASTNode(TyTable, TyNode); 
            symTableInsert(ValTable_, Param->Value, ParamType);
          }
        } 

        Type *ReturnTy = checkExpr(TyTable_, ValTable_, Expr),
             *DeclTy   = getTypeFromASTNode(TyTable, TyNode);
        if (!DeclTy) DeclTy = createType(AnswerTy, NULL);

        if (!typeEqual(TyTable, ReturnTy, DeclTy)) 
          semError(1, Node, "Type declared of function '%s' does not match.", Node->Value);
        destroyType(DeclTy);
      } break;
    default:
      break;
  }
  return 1;
}

/* <function> */
Type *checkExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  if (!Node) return NULL;

  PtrVector *V = &(Node->Child);
  switch (Node->Kind) {
    case IntLit:
      return createType(IntTy, NULL);
    case FloatLit:
      return createType(FloatTy, NULL);
    case StringLit:
      return createType(StringTy, NULL);
    case IdLval:
      if (symTableExists(ValTable, Node->Value)) 
        return symTableFind(ValTable, Node->Value);
      else semError(1, Node, "Identifier '%s' not declared.", Node->Value);
      break;
    case RecAccessLval:
      {
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
      } break;
    case ArrAccessLval:
      {
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
      } break;
    case SumOp:
    case SubOp:
    case MultOp:
    case DivOp:
      {
        Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)), 
             *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
        if (typeEqual(TyTable, E1, E2) && (E1->Kind == IntTy || E1->Kind == FloatTy))
          return E1;
        else semError(1, Node, "Arithmetic operation permited only on same type, Float or Int operands.");
      } break;
    case AndOp:
    case OrOp:
      {
        Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)), 
             *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
        if (typeEqual(TyTable, E1, E2) && E1->Kind == IntTy)
          return E1;
        else semError(1, Node, "Boolean operation permited only on Int type.");
      } break;
    case NegOp:
      {
        Type *ExprType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
        if (!ExprType) break;
        if (ExprType->Kind == IntTy || ExprType->Kind == FloatTy) return ExprType;
        else semError(1, Node, "Arithmetic operation permited only on same type, Float or Int operands.");
      } break;
    case LtOp:
    case LeOp:
    case GtOp:
    case GeOp:
      {
        Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)), 
             *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
        if (typeEqual(TyTable, E1, E2) && (E1->Kind == IntTy || E1->Kind == FloatTy || 
              E1->Kind == StringTy))
          return createType(IntTy, NULL);
        else semError(1, Node, "Inequality operation permited only on same type, Float, String or Int operands.");
      } break;
    case EqOp:
    case DiffOp:
      {
        Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)), 
             *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
        if (typeEqual(TyTable, E1, E2))
          return createType(IntTy, NULL);
        else semError(1, Node, "Operators must be of the same type.");
      } break;
    case LetExpr:
      {
        SymbolTable *TyTable_  = createSymbolTable(Node, TyTable),
                    *ValTable_ = createSymbolTable(Node, ValTable);
        if (!checkDecl(TyTable_, ValTable_, ptrVectorGet(V, 0))) break;
        Type *ExprType = checkExpr(TyTable_, ValTable_, ptrVectorGet(V, 1));
        return ExprType;
      } break;
    case IfStmtExpr:
      {
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
      } break;
    case FunCallExpr:
      {
        Type *FnType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
        FnType = resolveType(TyTable, FnType);
        if (FnType->Kind == FunTy) {
          Type *ArgType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
          if (typeEqual(TyTable, ((Type**) FnType->Val)[0], ArgType))
            return (Type*) ((Type**) FnType->Val)[1];
          else semError(1, Node, "Arguments do not match the call.");
        } else semError(1, Node, "Expression does not result in function to call.");
      } break;
    case ArgExprList:
      {
        PtrVector *Args;
        PtrVectorIterator I = beginPtrVector(V), 
                          E = endPtrVector(V);
        Args = createPtrVector();
        for (; I != E; ++I) 
          ptrVectorAppend(Args, checkExpr(TyTable, ValTable, *I));
        return createType(SeqTy, Args); 
      } break;
    case ArrayExpr:
      {
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
      } break;
    case RecordExpr:
      {
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
      }
    case FieldExprList:
      {
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
    case NilExpr:
      return createType(NilTy, NULL);
    default:
      break;
  }
  return NULL;
}

