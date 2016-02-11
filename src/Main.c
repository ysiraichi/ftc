

#include <stdio.h>

#include "llvm-c/Core.h"
#include "llvm-c/Target.h"

#include "ftc/Analysis/ParserTree.h"
#include "ftc/Analysis/DrawTree.h"
#include "ftc/Analysis/SymbolTable.h"
#include "ftc/Analysis/TypeCheck.h"
#include "ftc/Analysis/BaseEnviroment.h"
#include "ftc/LLVMCode/Translate.h"
#include "ftc/LLVMCode/BaseCode.h"

extern FILE    *yyin;
extern ASTNode *Root;

char MainFName[] = "main";
char MainFEscaped[] = "escaped.main";

int yyparse(void);

LLVMModuleRef  Module;
LLVMBuilderRef Builder;
int BBCount = 0;
int GVCount = 0;

void initLLVM() {
  Module = LLVMModuleCreateWithName("MyModule");

  LLVMTypeRef  MainFType    = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
  LLVMValueRef MainFunction = LLVMAddFunction(Module, "main", MainFType);

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(MainFunction, "entry");

  Builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(Builder, Entry);
}

void doTypeCheck() {
  ASTNode     *MainFunc = createASTNode(FunDecl, MainFName, 0);
  SymbolTable *BaseTy   = createSymbolTable(NULL, 0),
              *BaseVal  = createSymbolTable(NULL, 0);


  Hash *MainEscVars = createHash();
  BaseTy->Owner = MainFunc;
  BaseVal->Owner = MainFunc;
  symTableInsert(BaseVal, MainFEscaped, MainEscVars);

  addBaseEnviroment(BaseTy, BaseVal);
  checkExpr(BaseTy, BaseVal, Root);

  printf("EndOfSemantic.\n");

  initLLVM();
  addAllBaseTypes(BaseTy, LLVMGetModuleContext(Module) );
  LLVMValueRef DummyRA = createDummyActivationRecord(Builder);
  addAllBaseFunctions(BaseVal, DummyRA, MainEscVars);
  LLVMValueRef MainRA = createCompleteActivationRecord(Builder, MainEscVars);
  putRAHeadAhead(MainRA);
  heapPush(MainRA);

  LLVMValueRef MainF = getFunctionFromBuilder(Builder);
  LLVMBasicBlockRef Start = LLVMAppendBasicBlock(MainF, "program_start");
  LLVMBuildBr(Builder, Start);
  LLVMPositionBuilderAtEnd(Builder, Start);
  
  translateExpr(BaseTy, BaseVal, Root);
  heapPop();
  returnRAHead();
  LLVMBuildRet(Builder, getSConstInt(0));
  LLVMDumpModule(Module);
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

