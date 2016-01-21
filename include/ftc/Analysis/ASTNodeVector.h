#ifndef ASTNODE_VECTOR_H
#define ASTNODE_VECTOR_H

struct ASTNode;

struct ASTNodeVector {
  unsigned MaxSize, Size;
  ASTNode **Head;
}

void initASTNodeVector(ASTNodeVector *NVec, int Size) {
  NVec->Head = (ASTNode**) malloc(sizeof(ASTNode*) * Size);
  NVec->Last  = NVec->Head;
  NVec->Size  = 0;
  NVec->MaxSize = Size;
}

bool expandASTNodeVector(ASTNodeVector *NVec) {
  unsigned NewSize = 2 * NVec->MaxSize;
  ASTNode **NewHead = (ASTNode**) realloc(NVec->Head, NewSize * sizeof(ASTNode*));
  if (!NewHead) return false;
  NVec->Head   = NewHead;
  NVec->MaxSize = NewSize;
  return true;
}

bool appendASTNode(ASTNodeVector *NVec, ASTNode *Node) {
  if (Size == MaxSize && !expandASTNodeVector(NVec)) return false;
  NVec->Head[Size++] = Node;
  return true;
}

ASTNode **begin(ASTNodeVector *NVec) { return NVec->Head; }
ASTNode **end(ASTNodeVector *NVec) { return NVec->Head[Size]; }

#endif
