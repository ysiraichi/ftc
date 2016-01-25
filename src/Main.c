#include <stdio.h>

#include "ftc/Analysis/ParserTree.h"
#include "ftc/Analysis/DrawTree.h"

extern FILE    *yyin;
extern ASTNode *Root;

int yyparse(void);

int main(int argc, char **argv) {
  if (argc > 1) yyin = fopen(argv[1], "r");
  int parsing = yyparse();
  if (!parsing) {
    drawDotTree("MyDot.dot", Root);
    destroyASTNode(Root);
  }
  return parsing;
} 
