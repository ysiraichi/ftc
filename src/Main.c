

#include <stdio.h>
#include <string.h>

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

static char *OutputFileName;
static char *TreeFileName;

static char MainFName[] = "main";
static char MainFEscaped[] = "escaped.main";

int yyparse(void);

LLVMModuleRef  Module;
LLVMBuilderRef Builder;

void initLLVM() {
  Module = LLVMModuleCreateWithName("MyModule");

  LLVMTypeRef  MainFType    = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
  LLVMValueRef MainFunction = LLVMAddFunction(Module, "main", MainFType);

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(MainFunction, "entry");

  Builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(Builder, Entry);
}

void doCompile() {
  ASTNode     *MainFunc = createASTNode(FunDecl, MainFName, 0);
  SymbolTable *BaseTy   = createSymbolTable(NULL, 0),
              *BaseVal  = createSymbolTable(NULL, 0);

  /* Creating base enviroment for type check */
  Hash *MainEscVars = createHash();
  BaseTy->Owner = MainFunc;
  BaseVal->Owner = MainFunc;
  symTableInsert(BaseVal, MainFEscaped, MainEscVars);

  addBaseEnviroment(BaseTy, BaseVal);

  /* Type checking */
  checkExpr(BaseTy, BaseVal, Root);

  initLLVM();
  /* Creating base enviroment for translating */
  addAllBaseTypes(BaseTy, LLVMGetModuleContext(Module) );
  LLVMValueRef DummyRA = createDummyActivationRecord(Builder);
  addAllBaseFunctions(BaseVal, DummyRA, MainEscVars);
  LLVMValueRef MainRA = createCompleteActivationRecord(Builder, MainEscVars);

  /* Pushing 'main' activation record */
  heapPush(MainRA);

  /* Creating 'main' function */
  LLVMValueRef MainF = getFunctionFromBuilder(Builder);
  LLVMBasicBlockRef Start = LLVMAppendBasicBlock(MainF, "program_start");
  LLVMBuildBr(Builder, Start);
  LLVMPositionBuilderAtEnd(Builder, Start);

  /* Translating */
  translateExpr(BaseTy, BaseVal, Root);

  /* Popping 'main' activation record */
  heapPop();

  /* Finalizing 'main' */
  LLVMBuildRet(Builder, getSConstInt(0));
}

void printHelp() {
  printf("\n-------------------- The Pure-Functional Tiger Language Compiler --------------------------\n\n");
  printf("> Usage: ftc <inputFile> [-o <outputFile>] [-tree <treeOutputFile>]\n\n");

  printf("> Arguments:\n");
  printf("    . InputFile :: the file which contains a pure-functional tiger program;\n");
  printf("    . -o        :: the output file in which will contain the LLVM IR;\n");
  printf("    . -tree     :: the tree output file in which will contain the AST in 'dot' format;\n");
  printf("\n-------------------------------------------------------------------------------------------\n\n");
  exit(0);
}

typedef enum {
  NONE, OUTPUT, TREEPATH, HELP
} CommandArgsOpt;

void commandArgsParse(int argc, char **argv) {
  if (argc == 1) printHelp();

  yyin           = NULL;
  OutputFileName = NULL;

  int I;
  CommandArgsOpt O = NONE;
  for (I = 1; I < argc; ++I) {
    switch (O) {
      case NONE:
        {
          if (!strcmp("-o", argv[I])) O = OUTPUT;
          else if (!strcmp("-tree", argv[I])) O = TREEPATH;
          else if (!strcmp("-h", argv[I]) || !strcmp("--help", argv[I])) O = HELP;
          else yyin = fopen(argv[I], "r");
          break;
        }
      case OUTPUT:
        {
          OutputFileName = argv[I];
          O = NONE;
          break;
        }
      case TREEPATH:
        {
          TreeFileName = argv[I];
          O = NONE;
          break;
        }
      case HELP:
        {
          printHelp();
          O = NONE;
          break;
        }
    }
  }

  if (O != NONE) printHelp();

  if (!yyin) printHelp();
  if (!OutputFileName) {
    OutputFileName = (char*) malloc(sizeof(char) * 12);
    strcpy(OutputFileName, "/dev/stderr");
    OutputFileName[11] = '\0';
  }
  if (!TreeFileName) {
    TreeFileName = (char*) malloc(sizeof(char) * 12);
    strcpy(TreeFileName, "/dev/stdout");
    TreeFileName[11] = '\0';
  }
}

int main(int argc, char **argv) {
  commandArgsParse(argc, argv);
  if (!yyparse()) {
    drawDotTree(TreeFileName, Root);
    doCompile();
    LLVMPrintModuleToFile(Module, OutputFileName, NULL);
    destroyASTNode(Root);
    return 0;
  }
  return 1;
} 

