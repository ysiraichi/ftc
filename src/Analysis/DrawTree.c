#include "ftc/Analysis/DrawTree.h"

#include <stdio.h>
#include <stdarg.h>

static FILE *O;
static char Name[][64] = {
  "DeclList",
  "TyDeclList",
  "FunDeclList",
  "ArgDeclList",
  "ArgExprList",
  "FieldExprList",
  "VarDecl",
  "FunDecl",
  "TyDecl",
  "ArgDecl",
  "IntTy",
  "FloatTy",
  "StringTy",
  "AnswerTy",
  "ContTy",
  "StrConsumerTy",
  "IdTy",
  "FunTy",
  "ArrayTy",
  "RecordTy",
  "SeqTy",
  "IntLit",
  "FloatLit",
  "StringLit",
  "IdLval",
  "RecAccessLval",
  "ArrAccessLval",
  "SumOp",
  "SubOp",
  "MultOp",
  "DivOp",
  "EqOp",
  "DiffOp",
  "LtOp",
  "LeOp",
  "GtOp",
  "GeOp",
  "AndOp",
  "OrOp",
  "NegOp",
  "LetExpr",
  "IfStmtExpr",
  "FunCallExpr",
  "ArgExpr",
  "ArrayExpr",
  "RecordExpr",
  "FieldExpr"
};

static void createLeaf(void *Ptr, const char *S, ...) {
  fprintf(O, "Node%p [label=\"", Ptr);

  va_list Args;
  va_start(Args, S);
  vfprintf(O, S, Args);
  va_end(Args);

  fprintf(O, "\"];\n");
}

static void createNode(void *Ptr, const char *Name) {
  fprintf(O, "Node%p [shape=box,label=\"%s\"];\n", Ptr, Name);
}

static void createEdge(void *V1, void *V2) {
  fprintf(O, "Node%p -> Node%p;\n", V1, V2);
}

static void createNilEdge(void *V1) {
  fprintf(O, "Node%p -> NodeNull;\n", V1);
}

static void drawASTNode(ASTNode *Node) {
  PtrVector *Child    = &(Node->Child);
  PtrVectorIterator I = beginPtrVector(Child), 
                    E = endPtrVector(Child);
  void *Value = Node->Value;
  switch (Node->Kind) {
    case IntLit:
      createLeaf(Node, "Int: %d", *((int*)Value)); 
      break;
    case FloatLit:
      createLeaf(Node, "Float: %f", *((float*)Value)); 
      break;
    case StringLit:
      createLeaf(Node, "String: %s", (char*)Value); 
      break;
    case IdLval:
      createLeaf(Node, "Id: %s", (char*)Value); 
      break;
    case IntTy:
      createLeaf(Node, "Type: int"); 
      break;
    case FloatTy:
      createLeaf(Node, "Type: float"); 
      break;
    case StringTy:
      createLeaf(Node, "Type: string"); 
      break;
    case AnswerTy:
      createLeaf(Node, "Type: answer"); 
      break;
    case ContTy:
      createLeaf(Node, "Type: cont"); 
      break;
    case StrConsumerTy:
      createLeaf(Node, "Type: strConsumer"); 
      break;
    case IdTy:
      createLeaf(Node, "TypeId: %s", (char*)Value); 
      break;
    case NilExpr:
      createLeaf(Node, "Nil");
      break;
    default:
      createNode(Node, Name[Node->Kind]);
      if (Value) {
        createLeaf(Value, "Id: %s", (char*)Value);
        createEdge(Node, Value);
      }
      for (; I != E; ++I) 
        if (*I) {
          createEdge(Node, *I);
          drawASTNode(*I); 
        }
      
      break;
  }
}

void drawDotTree(const char *Filename, ASTNode *Root) {
  O = fopen(Filename, "w");
  fprintf(O, "digraph \"Abstract Syntatic Tree\" {\n");
  drawASTNode(Root);
  fprintf(O, "}\n");
}
