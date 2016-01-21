#include "ftc/Analysis/ParserTree.h"

#include <stdlib.h>
#include <stdarg.h>

/* function */
ASTNode *createASTNode(NodeKind K, void *Value, int N, ...) {
  int I;

  ASTNode *Node = (ASTNode*) malloc(sizeof(ASTNode));
  Node->Value = Value;
  Node->Kind  = K;

  initASTNodeVector(&(Node->Child));

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
  appendASTNode(&(Parent->Child), Child);
}

/* function */
void moveAllToASTNode(ASTNode *Node, ASTNodeVector *Vector) {
  ASTNodeVectorIterator I, E;
  for (I = beginASTNodeVector(Vector), E = endASTNodeVector(Vector); I != E; ++I)
    addToASTNode(Node, *I);
  destroyASTNodeVector(Vector);
}

/* function */
void destroyASTNode(ASTNode *Node) {
  if (Node->Value) free(Node->Value);

  ASTNodeVectorIterator I, E;
  ASTNodeVector *V = &(Node->Child);
  for (I = beginASTNodeVector(V), E = endASTNodeVector(V); I != E; ++I)
    if (*I) destroyASTNode(*I);
  destroyASTNodeVectorContents(V);

  free(Node);
}

