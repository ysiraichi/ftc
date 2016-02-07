#include <stdio.h>

#include "ftc/Analysis/ParserTree.h"
#include "ftc/Analysis/DrawTree.h"
#include "ftc/Analysis/SymbolTable.h"
#include "ftc/Analysis/TypeCheck.h"
#include "ftc/Analysis/BaseEnviroment.h"

extern FILE    *yyin;
extern ASTNode *Root;

char MainFName[] = "main";
char MainFEscaped[] = "escaped.main";

int yyparse(void);

void doTypeCheck() {
  ASTNode     *MainFunc = createASTNode(FunDecl, MainFName, 0);
  SymbolTable *BaseTy   = createSymbolTable(NULL, 0),
              *BaseVal  = createSymbolTable(NULL, 0);

  BaseTy->Owner = MainFunc;
  BaseVal->Owner = MainFunc;
  symTableInsert(BaseVal, MainFEscaped, createHash());
  
  addBaseEnviroment(BaseTy, BaseVal);
  checkExpr(BaseTy, BaseVal, Root);
}

int main(int argc, char **argv) {
  if (argc > 1) yyin = fopen(argv[1], "r");
  int parsing = yyparse();
  if (!parsing) {
    drawDotTree("MyDot.dot", Root);
    doTypeCheck();
    destroyASTNode(Root);
  }
  return parsing;
} 
