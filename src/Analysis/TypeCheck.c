#include "ftc/Analysis/TypeCheck.h"
#include "ftc/Analysis/Types.h"
#include "ftc/Analysis/SymbolTable.h"

#include <stdio.h>
#include <stdarg.h>

void checkDeclList(SymbolTable*, SymbolTable*, ASTNode*, int);
int checkDecl(SymbolTable*, SymbolTable*, ASTNode*);
Type *checkExpr(SymbolTable*, SymbolTable*, ASTNode*);

/* <function> */
static void semError(ASTNode *Node, const char *S, ...) {
  printf("@[%u,%u]: ", Node->Pos[0], Node->Pos[1]);
  va_list Args;
  va_start(Args, S);
  vprintf(S, Args);
  va_end(Args);
  printf("\n");
}

/* <function> */
static Type *getRealType(SymbolTable *TyTable, char *S) {
  Type *T = (Type*) symTableFind(TyTable, S);
  if (!T) return NULL;
  else if (T->Kind == IdTy) return getRealType(TyTable, T->Val);
  return createType(IdTy, S);
}

/* <function> */
static Type *getTypeFromASTNode(SymbolTable *TyTable, ASTNode *Node) {
  printf ("function: getTypeFromASTNode\n");
  if (!Node || (Node->Kind < ArgDecl && Node->Kind != ArgDeclList) || 
      Node->Kind > RecordTy) return NULL;

  if (Node->Kind < IntTy || Node->Kind > StrConsumerTy) {
    PtrVector *V = &(Node->Child);
    PtrVectorIterator I = beginPtrVector(V),
                      E = endPtrVector(V);
    switch (Node->Kind) {
      case IdTy:
        return getRealType(TyTable, Node->Value);
      case ArrayTy:
        {
          ASTNode *TyNode = (ASTNode*) ptrVectorGet(V, 0);
          Type *CoreTy = createType(TyNode->Kind, NULL);
          return createType(Node->Kind, CoreTy);
        }
      case FunTy:
        {
          Type **TyVec = (Type**) malloc(sizeof(Type*) * 2);
          TyVec[0] = getTypeFromASTNode(TyTable, ptrVectorGet(V, 0));
          TyVec[1] = getTypeFromASTNode(TyTable, ptrVectorGet(V, 1));
          return createType(Node->Kind, TyVec);
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
            hashInsert(Record, N->Value, getTypeFromASTNode(TyTable, TyNode)); 
          }
          return createType(RecordTy, Record);
        }
    }
  }

  return createType(Node->Kind, NULL);

}

/* <function> */
void checkDeclList(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node, int Mode) {
  PtrVector *V = &(Node->Child);
  PtrVectorIterator I = beginPtrVector(V),
                    E = endPtrVector(V);
  for (; I != E; ++I) {
    ASTNode *N = (ASTNode*) *I;
    if (Mode) symTableInsert(TyTable, N->Value, NULL);
    else symTableInsert(ValTable, N->Value, NULL);
  }

  for (I = beginPtrVector(V), E = endPtrVector(V); I != E; ++I) {
    checkDecl(TyTable, ValTable, *I);
  }
}

/* <function> */
int checkDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf ("function: checkDecl\n");
  PtrVector *V = &(Node->Child);
  PtrVectorIterator I = beginPtrVector(V),
                    E = endPtrVector(V);
  switch (Node->Kind) {
    case DeclList:
      for (; I != E; ++I) checkDecl(TyTable, ValTable, *I);
      break;
    case TyDeclList:
      checkDeclList(TyTable, ValTable, Node, 1);
      break;
    case FunDeclList:
      checkDeclList(TyTable, ValTable, Node, 0);
      break;
    case VarDecl:
      {
        ASTNode *TyNode = (ASTNode*) ptrVectorGet(V, 0),
                *Expr   = (ASTNode*) ptrVectorGet(V, 1);
        Type *ExprType  = checkExpr(TyTable, ValTable, Expr),
             *DeclType;
        if (!ExprType) return 0;
        if (TyNode) DeclType = getTypeFromASTNode(TyTable, TyNode);
        else DeclType = ExprType;
        if (compareType(ExprType, DeclType))
          symTableInsert(ValTable, Node->Value, DeclType); 
        else semError(Node, "Incompatible types of variable '%s'.", Node->Value);
        break;
      }
    case TyDecl:
      {
        ASTNode *N = (ASTNode*) ptrVectorGet(V, 0);
        int Exists = symTableExists(TyTable, Node->Value);
        if (N->Kind == IdTy && !symTableFind(TyTable, Node->Value) && Exists) 
          semError(Node, 
              "Recursive type '%s' not a record nor an array.\n \
              Type '%s' undefined.", Node->Value, N->Value);
        else if (N->Kind == IdTy && !Exists) 
          semError(Node, "Undefined type '%s'.", Node->Value);
        else symTableInsert(TyTable, Node->Value, getTypeFromASTNode(TyTable, N));
        break;
      }
    case FunDecl: 
      {
        ASTNode *Params = (ASTNode*) ptrVectorGet(V, 0),
                *TyNode = (ASTNode*) ptrVectorGet(V, 1),
                *Expr   = (ASTNode*) ptrVectorGet(V, 2);
        SymbolTable *TyTable_  = createSymbolTable(TyTable),
                    *ValTable_ = createSymbolTable(ValTable);
        PtrVector *Par;
        Type *ReturnTy, *ParamTy, *FunType, *DeclTy, **TyArr;

        if (!Params) ParamTy = NULL;
        else if (Params->Child.Size > 1) {
          Par = createPtrVector();
          PtrVectorIterator IPar = beginPtrVector(&(Params->Child)),
                            EPar = endPtrVector(&(Params->Child));
          for (; IPar != EPar; ++IPar) {
            ASTNode *Param  = (ASTNode*) *IPar,
                    *TyNode = (ASTNode*) ptrVectorGet(&(Param->Child), 0);
            Type *ParamType = getTypeFromASTNode(TyTable, TyNode); 
            symTableInsert(ValTable_, Param->Value, ParamType);
            ptrVectorAppend(Par, ParamType);
          }
          ParamTy = createType(SeqTy, Par);
        } else {
          ASTNode *Param  = (ASTNode*) ptrVectorGet(&(Params->Child), 0),
                  *TyNode = (ASTNode*) ptrVectorGet(&(Param->Child), 0);
          ParamTy = createType(TyNode->Kind, TyNode->Value);
          symTableInsert(ValTable_, Param->Value, ParamTy);
        } 

        ReturnTy = checkExpr(TyTable_, ValTable_, Expr);

        if (!ReturnTy) return 0;
        TyArr = (Type**) malloc(sizeof(Type*) * 2);
        TyArr[0] = ParamTy;
        TyArr[1] = ReturnTy;
        FunType = createType(FunTy, TyArr);
        if (TyNode) DeclTy = getTypeFromASTNode(TyTable, TyNode);
        else DeclTy = ReturnTy;
        if (compareType(ReturnTy, DeclTy)) {
          symTableInsert(ValTable, Node->Value, FunType); 
          printf("Inserted %s at %p.\n", (char*)Node->Value, (void*)ValTable);
        }
        else semError(Node, "Incompatible types of function '%s'.", Node->Value);
      } break;
  }
  return 1;
}

/* <function> */
Type *checkExpr(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  printf ("function: checkExpr\n");
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
      else semError(Node, "Identifier '%s' not declared.", Node->Value);
      break;
    case RecAccessLval:
      {
        Type *ExprType, *RealType;
        ExprType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
        if (!ExprType) break;
        RealType = symTableFind(TyTable, ExprType->Val);
        if (RealType->Kind == RecordTy) {
          Hash *RecordScope = (Hash*) RealType->Val;
          if (hashExists(RecordScope, Node->Value)) 
            return hashFind(RecordScope, Node->Value);
          else semError(Node, "Type '%s' has no member '%s'.", ExprType->Val, Node->Value);
        } else semError(Node, "Variable type is not a record.");
      } break;
    case ArrAccessLval:
      {
        Type *ExprType, *RealType, *IdxType;
        ExprType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
        if (!ExprType) break;
        RealType = symTableFind(TyTable, ExprType->Val);
        if (RealType->Kind == ArrayTy) {
          IdxType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
          if (!IdxType) break;
          if (IdxType->Kind == IntTy) {
            return (Type*) RealType->Val;
          } else semError(Node, "Index is not an integer number.");
        } else semError(Node, "Variable type is not an array.");
      } break;
    case SumOp:
    case SubOp:
    case MultOp:
    case DivOp:
      {
        Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)), 
             *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
        if (!E1 || !E2) break;
        if (compareType(E1, E2) && (E1->Kind == IntTy || E1->Kind == FloatTy))
          return E1;
        else semError(Node, "Arithmetic operation permited only on Float or Int type.");
      } break;
    case AndOp:
    case OrOp:
      {
        Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)), 
             *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
        if (!E1 || !E2) break;
        if (compareType(E1, E2) && E1->Kind == IntTy)
          return E1;
        else semError(Node, "Boolean operation permited only on Int type.");
      } break;
    case NegOp:
      {
        Type *ExprType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
        if (!ExprType) break;
        if (ExprType->Kind == IntTy || ExprType->Kind == FloatTy) return ExprType;
        else semError(Node, "Arithmetic operation permited only on Float or Int type.");
      } break;
    case LtOp:
    case LeOp:
    case GtOp:
    case GeOp:
      {
        Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)), 
             *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
        if (!E1 || !E2) break;
        if (compareType(E1, E2) && (E1->Kind == IntTy || E1->Kind == FloatTy || 
              E1->Kind == StringTy))
          return createType(IntTy, NULL);
        else semError(Node, "Inequality operation permited only on Float, Int or String type.");
      } break;
    case EqOp:
    case DiffOp:
      {
        Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0)), 
             *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
        if (!E1 || !E2) break;
        if (compareType(E1, E2))
          return createType(IntTy, NULL);
        else semError(Node, "Operators must be of the same type.");
      } break;
    case LetExpr:
      {
        SymbolTable *TyTable_  = createSymbolTable(TyTable),
                    *ValTable_ = createSymbolTable(ValTable);
        if (!checkDecl(TyTable_, ValTable_, ptrVectorGet(V, 0))) break;
        Type *ExprType = checkExpr(TyTable_, ValTable_, ptrVectorGet(V, 1));
        if (ExprType) return ExprType;
      } break;
    case IfStmtExpr:
      {
        Type *ExprType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
        if (!ExprType) break;
        if (ExprType->Kind == IntTy) {
          Type *E1 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1)), 
               *E2 = checkExpr(TyTable, ValTable, ptrVectorGet(V, 2));
          if (E2 && compareType(E1, E2)) return E1;
          else if (E2) semError(Node, 
              "Expressions after 'THEN' and 'ELSE' must be of the same type.");
          else if (E1 && E1->Kind == AnswerTy) return E1;
          else semError(Node, "Expression not returning 'answer' after 'THEN'.");
        } else semError(Node, "Condition does not result in integer.");
      } break;
    case FunCallExpr:
      {
        Type *FnType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
        if (!FnType) break;
        if (FnType->Kind == FunTy) {
          Type *ArgType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1));
          if (compareType(((Type**) FnType->Val)[0], ArgType))
            return (Type*) ((Type**) FnType->Val)[1];
          else semError(Node, "Arguments do not match the call.");
        } else semError(Node, "Expression does not result in function to call.");
      } break;
    case ArgExprList:
      {
        PtrVector *Args;
        if (V->Size > 1) {
          PtrVectorIterator I = beginPtrVector(V), 
                            E = endPtrVector(V);
          Args = createPtrVector();
          for (; I != E; ++I) 
            ptrVectorAppend(Args, checkExpr(TyTable, ValTable, *I));
          return createType(SeqTy, Args); 
        } else if (V->Size == 1) {
          return checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
        } else return NULL;
      } break;
    case ArrayExpr:
      {
        Type *ThisType = symTableFind(TyTable, Node->Value);
        if (ThisType) {
          if (ThisType->Kind == ArrayTy) {
            Type *SizeType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
            if (SizeType && SizeType->Kind == IntTy) {
              Type *InitType = checkExpr(TyTable, ValTable, ptrVectorGet(V, 1)); 
              if (InitType && compareType(ThisType->Val, InitType)) {
                return ThisType;
              } else semError(Node, "Init expression is not the same type as the array.");
            } else semError(Node, "Size expression is not an integer type.");
          } else semError(Node, "Type '%s' is not an array type.", Node->Value);
        } else semError(Node, "Type '%s' undefined.", Node->Value);
      } break;
    case RecordExpr:
      {
        PtrVectorIterator B, E;
        Type *ThisType = symTableFind(TyTable, Node->Value);
        if (ThisType) {
          if (ThisType->Kind == RecordTy) {
            int Success = 1;
            Type *FieldsTy = checkExpr(TyTable, ValTable, ptrVectorGet(V, 0));
            if (!FieldsTy) break;
            Hash *RecScope = (Hash*) ThisType->Val,
                 *Record   = (Hash*) FieldsTy->Val;
            if (RecScope->Pairs.Size == Record->Pairs.Size) {
              for (B = beginHash(RecScope), E = endHash(RecScope); B != E; ++B) {
                Pair *P = (Pair*) *B;
                Success = Success && hashExists(Record, P->Key) && 
                  compareType(P->Value, hashFind(Record, P->Key));
                if (!Success) break;
              }
              if (Success) return ThisType;
              else semError(Node, "Record '%s' did not match call.", Node->Value);
            } else semError(Node, "Record '%s' did not match call.", Node->Value);
          } else semError(Node, "Type '%s' is not a record.", Node->Value);
        } else semError(Node, "Type '%s' undefined.", Node->Value);
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
  }
  return NULL;
}

