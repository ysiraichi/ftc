

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Initialization.h"

#include "ftc/Analysis/ParserTree.h"
#include "ftc/Analysis/DrawTree.h"
#include "ftc/Analysis/SymbolTable.h"
#include "ftc/Analysis/TypeCheck.h"
#include "ftc/Analysis/BaseEnviroment.h"
#include "ftc/LLVMCode/Translate.h"
#include "ftc/LLVMCode/BaseCode.h"

typedef enum {
  NONE, OUTPUT, HELP
} CommandArgsOpt;

extern FILE    *yyin;
extern ASTNode *Root;

static char OutputFileName[128];

static int PrintLLVM = 0;
static int PrintAssembly = 0;
static int PrintASTree = 0;

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

void doAnalysisToLLVMIR() {
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

void doCompileLLVMIRtoAssembly() {
  /*
     LLVMPassRegistryRef Registry = LLVMGetGlobalPassRegistry();
     LLVMInitializeCore(Registry);
     */
  LLVMPassRegistryRef Registry = LLVMGetGlobalPassRegistry();
  LLVMInitializeCore(Registry);
  LLVMInitializeCodeGen(Registry);
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllAsmPrinters();
  LLVMInitializeAllAsmParsers();

  char *Triple, *Error, CPU[] = "generic", Features[] = "", OutputAssembly[128];
  LLVMBool Result;
  LLVMTargetRef Target;
  LLVMTargetMachineRef TargetMachine;

  strcpy(OutputAssembly, OutputFileName);
  strcat(OutputAssembly, ".s");

  Triple = LLVMGetDefaultTargetTriple();
  LLVMGetTargetFromTriple(Triple, &Target, &Error);
  if (!Target) printf("Error: %s\n", Error);
  TargetMachine = LLVMCreateTargetMachine(Target, Triple, CPU, Features, 
      LLVMCodeGenLevelNone, LLVMRelocDefault, LLVMCodeModelDefault);

  Result = LLVMTargetMachineEmitToFile(TargetMachine, Module, OutputAssembly, LLVMAssemblyFile, &Error);
  if (Result) printf("Error: %s\n", Error);

}

void printHelp() {
  printf("\n-------------------- The Pure-Functional Tiger Language Compiler --------------------------\n\n");
  printf("> Usage: ftc <inputFile> [-o <outputFile>] [-tree] [-emit-llvm ] [-emit-as]\n\n");

  printf("> Arguments:\n");
  printf("    . InputFile  :: the file which contains a pure-functional tiger program;\n");
  printf("    . -o         :: the output file base name;\n");
  printf("    . -tree      :: prints the program's ASTree in a 'dot' file graph;\n");
  printf("    . -emit-llvm :: emits a file corresponding to the program in LLVM IR language;\n");
  printf("    . -emit-as   :: emits a file corresponding to the program in Assembly language;\n");
  printf("\n-------------------------------------------------------------------------------------------\n\n");
  exit(0);
}

void commandArgsParse(int argc, char **argv) {
  if (argc == 1) printHelp();

  yyin = NULL;
  OutputFileName[0] = '\0';

  int I;
  CommandArgsOpt O = NONE;
  for (I = 1; I < argc; ++I) {
    switch (O) {
      case NONE:
        {
          if (!strcmp("-o", argv[I])) O = OUTPUT;
          else if (!strcmp("-h", argv[I]) || !strcmp("--help", argv[I])) O = HELP;
          else if (!strcmp("-tree", argv[I])) PrintASTree = 1;
          else if (!strcmp("-emit-llvm", argv[I])) PrintLLVM = 1;
          else if (!strcmp("-emit-as", argv[I])) PrintAssembly = 1;
          else yyin = fopen(argv[I], "r");
          break;
        }
      case OUTPUT:
        {
          strcpy(OutputFileName, argv[I]);
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
  if (!OutputFileName[0]) {
    strcpy(OutputFileName, "a");
    OutputFileName[2] = '\0';
  }
}

int main(int argc, char **argv) {
  commandArgsParse(argc, argv);
  if (!yyparse()) {
    if (PrintASTree) {
      char TreeFileName[128];
      sprintf(TreeFileName, "%s.dot", OutputFileName);
      drawDotTree(TreeFileName, Root);
    }

    doAnalysisToLLVMIR();

    char LLVMIRFile[128], ExeFile[128], Command[256];
    if (!strcmp(OutputFileName, "a")) 
      sprintf(ExeFile, "%s.out", OutputFileName);
    else strcpy(ExeFile, OutputFileName);
    sprintf(LLVMIRFile, "%s.ll", OutputFileName);

    LLVMPrintModuleToFile(Module, LLVMIRFile, NULL);
    sprintf(Command, "clang-3.7 %s -o %s", LLVMIRFile, ExeFile);
    system(Command); 

    if (PrintAssembly) doCompileLLVMIRtoAssembly();
    if (!PrintLLVM)    remove(LLVMIRFile);
    destroyASTNode(Root);
    return 0;
  }
  return 1;
} 

