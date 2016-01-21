#include "ftc/Analysis/DrawDotTree.h"

#include <stdarg.h>
#include <stdio.h>

static void drawLitT(LitT*);
static void drawLvalT(LvalT*);
static void drawBinOpT(BinOpT*);
static void drawNegT(NegT*);
static void drawLetT(LetT*);
static void drawDeclT(DeclT*);
static void drawVarDeclT(VarDeclT*);
static void drawFunDeclT(FunDeclT*);
static void drawTyDeclT(TyDeclT*);
static void drawTypeT(TypeT*);
static void drawParamTyT(ParamTyT*);
static void drawArrayTyT(ArrayTyT*);
static void drawSeqTyT(SeqTyT*);
static void drawFunTyT(FunTyT*);
static void drawIfStmtT(IfStmtT*);
static void drawArgT(ArgT*);
static void drawFunCallT(FunCallT*);
static void drawCreateT(CreateT*);
static void drawArrayT(ArrayT*);
static void drawFieldT(FieldT*);
static void drawRecordT(RecordT*);
static void drawNilT(NilT*);
static void drawExprT(ExprT*);

static FILE *O;
static const char *Leaf = "Leaf";
static int Count = 0;

static void printNode(void *Node, const char *Label) {
  fprintf(O, "  Node%p [shape=box,label=\"%s\"];\n", Node, Label);
}

static void printLeaf(char *Format, ...) {
  va_list Args;
  va_start(Args, Format);
  fprintf(O, "  %s%d [label=\"", Leaf, ++Count);
  vfprintf(O, Format, Args);
  fprintf(O, "\"];\n");
  va_end(Args);
}

static void printEdge(void *One, void *Two) {
  if (!Two) fprintf(O, "  Node%p -> %s%d;\n", One, Leaf, Count);
  else fprintf(O, "  Node%p -> Node%p;\n", One, Two);
}

static void drawLitT(LitT *L) {
  printNode(L, "Literal");
  switch (L->Kind) {
    case Int:    printLeaf("Int: %d", L->Value.Int);       break;
    case Float:  printLeaf("Float: %lf", L->Value.Float);  break;
    case String: printLeaf("String: %s", L->Value.String); break;
  }
  printEdge(L, NULL);
}

static void drawLvalT(LvalT *L) {
  printNode(L, "Lval");
  switch (L->Kind) {
    case Id:
      printLeaf("Id: %s", L->Id);
      printEdge(L, NULL);
      break;
    case ArrayAccess:  
      if (L->Lval) {
        printEdge(L, L->Lval);
        drawLvalT(L->Lval);
      }
      drawExprT(L->Pos);
      break;
    case RecordAccess: 
      if (L->Lval) {
        printEdge(L, L->Lval);
        drawLvalT(L->Lval);
      }
      printLeaf("Id: %s", L->Id);
      printEdge(L, NULL);
      break;
  }
}

static void drawBinOpT(BinOpT *B) {
  switch (B->Kind) {
    case Sum:  printNode(B, "Sum");  break;
    case Sub:  printNode(B, "Sub");  break;
    case Mult: printNode(B, "Mult"); break;
    case Div:  printNode(B, "Div");  break;
    case Eq:   printNode(B, "Eq");   break;
    case Diff: printNode(B, "Diff"); break;
    case Lt:   printNode(B, "Lt");   break;
    case Le:   printNode(B, "Le");   break;
    case Gt:   printNode(B, "Gt");   break;
    case Ge:   printNode(B, "Ge");   break;
    case And:  printNode(B, "And");  break;
    case Or:   printNode(B, "Or");   break;
  }
  printEdge(B, B->ExprOne);
  printEdge(B, B->ExprTwo);
  drawExprT(B->ExprOne);
  drawExprT(B->ExprTwo);
}

static void drawNegT(NegT *N) {
  printNode(N, "Negation"); 
  printEdge(N, N->Expr);
  drawExprT(N->Expr);
}

static void drawLetT(LetT *L) {
  printNode(L, "Let");
  printEdge(L, L->Decl);
  printEdge(L, L->Expr);
  drawDeclT(L->Decl);
  drawExprT(L->Expr);
}

static void drawDeclT(DeclT *D) {
  printNode(D, "Declaration"); 
  switch (D->Kind) {
    case Var:
      printEdge(D, D->Value.Var);
      drawVarDeclT(D->Value.Var);
      break;
    case Fun:
      printEdge(D, D->Value.Fun);
      drawFunDeclT(D->Value.Fun);
      break;
    case Ty:
      printEdge(D, D->Value.Ty);
      drawTyDeclT(D->Value.Ty);
      break;
  }
  if (D->Next) {
    printEdge(D, D->Next);
    drawDeclT(D->Next);
  }
}

static void drawVarDeclT(VarDeclT *V) {
  printNode(V, "Variable Declaration");
  printLeaf("Var: %s", V->Id);
  printEdge(V, NULL);
  if (V->Expr) {
    printEdge(V, V->Expr);
    drawExprT(V->Expr);
  }
}

static void drawFunDeclT(FunDeclT *F) {
  printNode(F, "Function Declaration");
  printLeaf("Id: %s", F->Id);
  printEdge(F, NULL);
  if (F->Params) {
    printEdge(F, F->Params);
    drawParamTyT(F->Params);
  }
  printEdge(F, F->Expr);
  drawExprT(F->Expr);
  if (F->Next) {
    printEdge(F, F->Next);
    drawFunDeclT(F->Next);
  }
}

static void drawTyDeclT(TyDeclT *T) {
  printNode(T, "Type Declaration"); 
  printLeaf("Id: %s", T->Id);
  printEdge(T, NULL);
  printEdge(T, T->Type);
  drawTypeT(T->Type);
}

static void drawTypeT(TypeT *T) {
  printNode(T, "Type"); 
  switch (T->Kind) {
    case Int:
      printLeaf("int");
      printEdge(T, NULL);
      break;
    case Float:
      printLeaf("float");
      printEdge(T, NULL);
      break;
    case String:
      printLeaf("string");
      printEdge(T, NULL);
      break;
    case Answer:
      printLeaf("answer");
      printEdge(T, NULL);
      break;
    case Cont:
      printLeaf("cont");
      printEdge(T, NULL);
      break;
    case StrConsumer:
      printLeaf("stringConsumer");
      printEdge(T, NULL);
      break;
    case Name:
      printLeaf("Id: %s", T->Ty.Id);
      printEdge(T, NULL);
      break;
    case FunTy:
      printEdge(T, T->Ty.FunTy);
      drawFunTyT(T->Ty.FunTy);
      break;
    case ParamTy:
      printEdge(T, T->Ty.ParamTy);
      drawParamTyT(T->Ty.ParamTy);
      break;
    case ArrayTy:
      printEdge(T, T->Ty.ArrayTy);
      drawArrayTyT(T->Ty.ArrayTy);
      break;
  }
}

static void drawParamTyT(ParamTyT *T) {
  printNode(T, "Param Type"); 
  printLeaf("Id: %s", T->Id);
  printEdge(T, NULL);
  printEdge(T, T->Type);
  drawTypeT(T->Type);
  if (T->Next) {
    printEdge(T, T->Next);
    drawParamTyT(T->Next);
  }
}

static void drawArrayTyT(ArrayTyT *A) {
  printNode(A, "Array Type"); 
  printEdge(A, A->Type);
  drawTypeT(A->Type);
}

static void drawSeqTyT(SeqTyT *S) {
  printNode(S, "Sequence Type"); 
  printEdge(S, S->Type);
  drawTypeT(S->Type);
  if (S->Next) {
    printEdge(S, S->Next);
    drawSeqTyT(S->Next);
  }
}

static void drawFunTyT(FunTyT *F) {
  printNode(F, "Functional Type"); 
  if (F->From) {
    printEdge(F, F->From);
    drawSeqTyT(F->From);
  }
  printEdge(F, F->To);
  drawSeqTyT(F->To);
}

static void drawIfStmtT(IfStmtT *I) {
  printNode(I, "If Statement"); 
  printEdge(I, I->If);
  drawExprT(I->If);
  printEdge(I, I->Then);
  drawExprT(I->Then);
  if (I->Else) {
    printEdge(I, I->Else);
    drawExprT(I->Else);
  }
}

static void drawArgT(ArgT *A) {
  printNode(A, "Arguments"); 
  printEdge(A, A->Expr);
  drawExprT(A->Expr);
  if (A->Next) {
    printEdge(A, A->Next);
    drawArgT(A->Next);
  }
}

static void drawFunCallT(FunCallT *F) {
  printNode(F, "Function Call"); 
  printLeaf("Id: %s", F->Id);
  printEdge(F, NULL);
  if (F->Args) {
    printEdge(F, F->Args);
    drawArgT(F->Args);
  }
}

static void drawCreateT(CreateT *C) {
  printNode(C, "Create"); 
  switch (C->Kind) {
    case Record:
      printEdge(C, C->Value.Record);
      drawRecordT(C->Value.Record);
      break;
    case Array:
      printEdge(C, C->Value.Array);
      drawArrayT(C->Value.Array);
      break;
  }
}

static void drawArrayT(ArrayT *A) {
  printNode(A, "Create Array"); 
  printLeaf("Id: %s", A->Id);
  printEdge(A, NULL);
  printEdge(A, A->Size);
  printEdge(A, A->Value);
  drawExprT(A->Size);
  drawExprT(A->Value);
}

static void drawFieldT(FieldT *F) {
  printNode(F, "Record Fields"); 
  printLeaf("Id: %s", F->Id);
  printEdge(F, NULL);
  printEdge(F, F->Expr);
  drawExprT(F->Expr);
  if (F->Next) {
    printEdge(F, F->Next);
    drawFieldT(F->Next);
  }
}

static void drawRecordT(RecordT *R) {
  printNode(R, "Record"); 
  printLeaf("Id: %s", R->Id);
  printEdge(R, NULL);
  printEdge(R, R->Field);
  drawFieldT(R->Field);
}

static void drawNilT(NilT *N) {
  printNode(N, "Nil"); 
}

static void drawExprT(ExprT *E) {
  printNode(E, "Expression");
  switch (E->Kind) {
    case Lit:
      printEdge(E, E->Value.Lit);
      drawLitT(E->Value.Lit);
      break;
    case Lval:
      printEdge(E, E->Value.Lval);
      drawLvalT(E->Value.Lval);
      break;
    case BinOp:
      printEdge(E, E->Value.BinOp);
      drawBinOpT(E->Value.BinOp);
      break;
    case Neg:
      printEdge(E, E->Value.Neg);
      drawNegT(E->Value.Neg);
      break;
    case Let:
      printEdge(E, E->Value.Let);
      drawLetT(E->Value.Let);
      break;
    case IfStmt:
      printEdge(E, E->Value.IfStmt);
      drawIfStmtT(E->Value.IfStmt);
      break;
    case FunCall:
      printEdge(E, E->Value.FunCall);
      drawFunCallT(E->Value.FunCall);
      break;
    case Create:
      printEdge(E, E->Value.Create);
      drawCreateT(E->Value.Create);
      break;
    case Nil:
      printEdge(E, E->Value.Nil);
      drawNilT(E->Value.Nil);
      break;
  }
}

void drawDotTree(char *Filename, ExprT *E) {
  O = fopen(Filename, "w");

  fprintf(O, "digraph \"Abstract Syntax Tree\" {\n");
  drawExprT(E);
  fprintf(O, "}\n");
}
