#ifndef PARSERTREE_H
#define PARSERTREE_H

#include "ftc/Analysis/ASTNodeVector.h"

#define VEC_SIZE 2

typedef struct ASTNode ASTNode;

typedef enum {
  DeclList = 0, FunDeclList, ArgDeclList, TySeqList, ArgExprList, FieldExprList,

  /* Declarations */
  VarDecl, FunDecl, TyDecl, ArgDecl,
  TySeq,
  IntTy, FloatTy, StringTy, AnswerTy, ContTy, StrConsumerTy, 
  IdTy, FunTy, ArrayTy, 

  /* Expressions */
  IntLit, FloatLit, StringLit, 
  IdLval, RecAccessLval, ArrAccessLval,
  SumOp, SubOp, MultOp, DivOp, EqOp, DiffOp, LtOp, LeOp, GtOp, GeOp, AndOp, OrOp,
  NegOp, LetExpr, IfStmtExpr, FunCallExpr, ArgExpr, ArrayExpr, RecordExpr, FieldExpr,
  NilExpr
} NodeKind;

struct ASTNode {
  NodeKind Kind;
  void *Value;
  ASTNodeVector Child;
};

ASTNode *createASTNode   (NodeKind, void*, int,...);
void     destroyASTNode  (ASTNode*);
void     addToASTNode    (ASTNode*, ASTNode*);
void     moveAllToASTNode(ASTNode*, ASTNodeVector*);


#endif
