#ifndef TYPES_H
#define TYPES_H

struct Type {
  NodeKind Kind;
  union {
    Type      Ty; 
    Type      *Fn[2];
    PtrVector Seq;
  } V;
}

#endif
