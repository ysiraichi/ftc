#include "ftc/Analysis/TypeCheck.h"
#include "ftc/Analysis/Types.h"
#include "ftc/Analysis/SymbolTable.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

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
        {
          if (symTableExists(TyTable, Node->Value))
            return createType(IdTy, Node->Value);
          semError(1, Node, "Undefined type '%s'.", Node->Value);
        }
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
static void checkCycle(SymbolTable *TyTable, ASTNode *Start) {
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

static void checkTyDeclList(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);
  PtrVectorIterator I = beginPtrVector(V),
                    E = endPtrVector(V);

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
}

static void checkFunDeclList(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);
  PtrVectorIterator I = beginPtrVector(V),
                    E = endPtrVector(V);

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
}

static void checkVarDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);
  ASTNode *TyNode = (ASTNode*) ptrVectorGet(V, 0),
          *Expr   = (ASTNode*) ptrVectorGet(V, 1);
  Type *ExprType  = checkExpr(TyTable, ValTable, Expr),
       *DeclType;

  if (TyNode) DeclType = getTypeFromASTNode(TyTable, TyNode);
  else DeclType = ExprType;

  if (ExprType->Kind == NilTy && !getRecordType(TyTable, DeclType)) semError(1, Node, 
      "Initializing var '%s', which is not a record, with 'nil'.", Node->Value);

  checkIfNilTy(ExprType, DeclType);

  if (typeEqual(TyTable, ExprType, DeclType))
    symTableInsertOrChange(ValTable, Node->Value, createType(DeclType->Kind, DeclType->Val)); 
  else semError(1, Node, "Incompatible types of variable '%s'.", Node->Value);
}

static void checkTyDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);
  ASTNode *TyNode = (ASTNode*) ptrVectorGet(V, 0);
  Type    *RTy    = getTypeFromASTNode(TyTable, TyNode);

  if (TyNode->Kind == IdTy && !symTableExists(TyTable, TyNode->Value)) 
    semError(1, Node, "Undefined type '%s'.", Node->Value);

  symTableInsertOrChange(TyTable, Node->Value, RTy);
}

static void checkFunDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);
  ASTNode *Params = (ASTNode*) ptrVectorGet(V, 0),
          *TyNode = (ASTNode*) ptrVectorGet(V, 1),
          *Expr   = (ASTNode*) ptrVectorGet(V, 2);
  SymbolTable *TyTable_  = createSymbolTable(Node, TyTable),
              *ValTable_ = createSymbolTable(Node, ValTable);

  // Preparing structure for escaping variables.
  char Buf[NAME_MAX], *EscapedName;
  toEscapedName(Buf, Node->Value);
  Hash *EscapingVar = createHash();

  EscapedName = (char*) malloc(sizeof(char) * (strlen(Buf) + 1));
  strcpy(EscapedName, Buf);

  symTableInsert(ValTable_, EscapedName, EscapingVar);

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
}

/* <function> */
void checkDecl(SymbolTable *TyTable, SymbolTable *ValTable, ASTNode *Node) {
  PtrVector *V = &(Node->Child);
  PtrVectorIterator I = beginPtrVector(V),
                    E = endPtrVector(V);
  switch (Node->Kind) {
    case DeclList: for (; I != E; ++I) checkDecl(TyTable, ValTable, *I); break;;

    case TyDeclList:  checkTyDeclList (TyTable, ValTable, Node); break;
    case FunDeclList: checkFunDeclList(TyTable, ValTable, Node); break;

    case VarDecl: checkVarDecl(TyTable, ValTable, Node); break;
    case TyDecl:  checkTyDecl (TyTable, ValTable, Node); break;
    case FunDecl: checkFunDecl(TyTable, ValTable, Node); break;
    
    default: break;
  }
}
