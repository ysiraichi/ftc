#include "ftc/Analysis/ParserTree.h"

#include <stdlib.h>
#include <stdarg.h>

/* function */
ASTNode *createASTNode(NodeKind K, void *Value, int N, ...) {
  int I;

  ASTNode *Node = (ASTNode*) malloc(sizeof(ASTNode));
  Node->Value = Value;
  Node->Kind  = K;
  Node->EscapedLevel = 0;

  initPtrVector(&(Node->Child), 0);

  va_list Childrem;
  va_start(Childrem, N);
  for (I = 0; I < N; ++I) {
    ASTNode *Ptr = va_arg(Childrem, ASTNode*);
    addToASTNode(Node, Ptr);
  }
  va_end(Childrem);
  return Node;
}

/* function */
void addToASTNode(ASTNode *Parent, ASTNode *Child) {
  ptrVectorAppend(&(Parent->Child), Child);
}

/* function */
void moveAllToASTNode(ASTNode *Node, PtrVector *Vector) {
  PtrVectorIterator I, E;
  for (I = beginPtrVector(Vector), E = endPtrVector(Vector); I != E; ++I)
    addToASTNode(Node, *I);
  destroyPtrVector(Vector, NULL);
}

/* function */
void destroyASTNode(void *V) {
  if (!V) return;
  ASTNode *Node = (ASTNode*) V;
  if (Node->Value) free(Node->Value);
  destroyPtrVectorContents(&(Node->Child), &destroyASTNode);
  free(Node);
}

void setASTNodePos(ASTNode *Node, unsigned Line, unsigned Col) {
  Node->Pos[0] = Line;
  Node->Pos[1] = Col;
}
