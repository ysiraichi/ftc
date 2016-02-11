#include "ftc/LLVMCode/BaseCode.h"

#include <string.h>

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

static LLVMTypeRef getContFunctionPtrType() {
  LLVMTypeRef FnType = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
  return LLVMPointerType(FnType, 0);
}

static LLVMTypeRef getStringConsumerFunctionPtrType() {
  LLVMTypeRef ArgType[] = { LLVMPointerType(LLVMInt8Type(), 0) };
  LLVMTypeRef FnType = LLVMFunctionType(LLVMVoidType(), ArgType, 1, 0);
  return LLVMPointerType(FnType, 0);
}

static void addLLVMMemCpyFunction() {
  LLVMTypeRef MemCpyParams[] = { 
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMInt32Type(), 
    LLVMInt32Type(), 
    LLVMIntType(1) 
  };

  LLVMTypeRef  MemCpyType     = LLVMFunctionType(LLVMVoidType(), MemCpyParams, 5, 0); 
  LLVMAddFunction(Module, "llvm.memcpy.p0i8.p0i8.i32", MemCpyType);
}

static void addStdFunctions() {
  /* newline */
  LLVMValueRef GlbNl = LLVMAddGlobal(Module, LLVMArrayType(LLVMInt8Type(), 2), "new.line");
  LLVMSetInitializer(GlbNl, LLVMConstString("\n", 1, 0));

  /* printf */
  LLVMTypeRef PrintfParams[] = {
    LLVMPointerType(LLVMInt8Type(), 0)
  };

  LLVMTypeRef  PrintfType     = LLVMFunctionType(LLVMInt32Type(), PrintfParams, 1, 1); 
  LLVMAddFunction(Module, "printf", PrintfType);

  /* strcmp */
  LLVMTypeRef StrCmpParams[] = {
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMPointerType(LLVMInt8Type(), 0)
  };

  LLVMTypeRef  StrCmpType     = LLVMFunctionType(LLVMInt32Type(), StrCmpParams, 1, 0); 
  LLVMAddFunction(Module, "strcmp", StrCmpType);

  /* getchar */
  LLVMTypeRef  GetCharType     = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 1); 
  LLVMAddFunction(Module, "getchar", GetCharType);

  /* fflush */
  LLVMContextRef Context = LLVMGetModuleContext(Module);
  LLVMTypeRef    IOFile  = LLVMStructCreateNamed(Context, "struct._IO_FILE");

  LLVMValueRef GlbStdout = LLVMAddGlobal(Module, LLVMPointerType(IOFile, 0), "stdout");
  LLVMSetExternallyInitialized(GlbStdout, 0);

  LLVMTypeRef FlushParams[] = { 
    LLVMPointerType(IOFile, 0)
  };  
  LLVMTypeRef  FlushType     = LLVMFunctionType(LLVMInt32Type(), FlushParams, 1, 0); 
  LLVMAddFunction(Module, "fflush", FlushType);

  /* exit */
  LLVMTypeRef ExitParams[] = { 
    LLVMInt32Type()
  };  
  LLVMTypeRef  ExitType     = LLVMFunctionType(LLVMVoidType(), ExitParams, 1, 0); 
  LLVMAddFunction(Module, "exit", ExitType);

  /* strlen */
  LLVMTypeRef StrLenParams[] = { 
    LLVMPointerType(LLVMInt8Type(), 0)
  };  
  LLVMTypeRef  StrLenType     = LLVMFunctionType(LLVMInt32Type(), StrLenParams, 1, 0); 
  LLVMAddFunction(Module, "strlen", StrLenType);
}

static void addPrintFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "print", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 6);
  strcpy(EscName, "print");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMPointerType(LLVMGetTypeByName(Module, "struct.Closure"), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMVoidType(), Params, 2, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "print", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef PrintfFn   = LLVMGetNamedFunction(Module, "printf");
  LLVMValueRef Args[]     = { LLVMGetParam(Function, 0) };
  LLVMBuildCall(Builder, PrintfFn, Args, 1, "");

  LLVMValueRef GlbNl   = LLVMGetNamedGlobal(Module, "new.line");
  LLVMValueRef GlbCast = LLVMBuildBitCast(Builder, GlbNl, LLVMPointerType(LLVMInt8Type(), 0), "");
  LLVMBuildCall(Builder, PrintfFn, &GlbCast, 1, "");

  LLVMValueRef ContFn  = LLVMGetParam(Function, 1);
  callClosure(getContFunctionPtrType(), ContFn, NULL, 0); 

  LLVMBuildRetVoid(Builder);
}

static void addGetCharFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "getchar", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 8);
  strcpy(EscName, "getchar");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMGetTypeByName(Module, "struct.Closure"), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "getcharTiger", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef GetCharFn   = LLVMGetNamedFunction(Module, "getchar");
  LLVMValueRef CallGetChar = LLVMBuildCall(Builder, GetCharFn, NULL, 0, "");
  LLVMValueRef CharValue   = LLVMBuildTrunc(Builder, CallGetChar, LLVMInt8Type(), "");

  LLVMValueRef String = LLVMBuildArrayMalloc(Builder, LLVMInt8Type(), getSConstInt(2), "");
  // String[0] = getChar()
  LLVMValueRef FstIdx[]   = { getSConstInt(0) };
  LLVMValueRef FstCharPtr = LLVMBuildInBoundsGEP(Builder, String, FstIdx, 1, "");
  LLVMBuildStore(Builder, CharValue, FstCharPtr);

  // String[1] = '\0'
  LLVMValueRef SndIdx[] = { getSConstInt(1) };
  LLVMValueRef SndCharPtr = LLVMBuildInBoundsGEP(Builder, String, SndIdx, 1, "");
  LLVMBuildStore(Builder, LLVMConstInt(LLVMInt8Type(), 0, 1), SndCharPtr);

  LLVMValueRef Closure = LLVMGetParam(Function, 0);
  callClosure(getStringConsumerFunctionPtrType(), Closure, &String, 1);

  LLVMBuildRet(Builder, String);
}

static void addFlushFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "flush", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 6);
  strcpy(EscName, "flush");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMGetTypeByName(Module, "struct.Closure"), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMVoidType(), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "flush", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef FlushFn   = LLVMGetNamedFunction(Module, "fflush");
  LLVMValueRef GlbStdout = LLVMGetNamedGlobal(Module, "stdout");
  LLVMValueRef GlbLoad   = LLVMBuildLoad(Builder, GlbStdout, "");
  LLVMBuildCall(Builder, FlushFn, &GlbLoad, 1, "");

  LLVMValueRef Closure = LLVMGetParam(Function, 0);
  callClosure(getContFunctionPtrType(), Closure, NULL, 0);

  LLVMBuildRetVoid(Builder);
}

static void addExitFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "exit", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 5);
  strcpy(EscName, "exit");

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "exitTiger", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef ExitFn   = LLVMGetNamedFunction(Module, "exit");
  LLVMValueRef Params[]  = { getSConstInt(0) };
  LLVMBuildCall(Builder, ExitFn, Params, 1, "");

  LLVMBuildRetVoid(Builder);
}

static void addOrdFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "ord", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 4);
  strcpy(EscName, "ord");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMInt32Type(), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "ord", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef FstChar  = LLVMBuildLoad(Builder, LLVMGetParam(Function, 0), "");
  LLVMValueRef RetValue = LLVMBuildSExt(Builder, FstChar, LLVMInt32Type(), "");

  LLVMBuildRet(Builder, RetValue);
}

static void addChrFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "chr", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 4);
  strcpy(EscName, "chr");

  LLVMTypeRef Params[] = {
    LLVMInt32Type()
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "chr", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef String  = LLVMBuildArrayMalloc(Builder, LLVMInt8Type(), getSConstInt(2), "");
  LLVMValueRef NewChar = LLVMBuildTrunc(Builder, LLVMGetParam(Function, 0), LLVMInt8Type(), "");

  // String[0] = fParam(0)
  LLVMValueRef FstIdx[]   = { getSConstInt(0) };
  LLVMValueRef FstCharPtr = LLVMBuildInBoundsGEP(Builder, String, FstIdx, 1, "");
  LLVMBuildStore(Builder, NewChar, FstCharPtr);

  // String[1] = '\0'
  LLVMValueRef SndIdx[] = { getSConstInt(1) };
  LLVMValueRef SndCharPtr = LLVMBuildInBoundsGEP(Builder, String, SndIdx, 1, "");
  LLVMBuildStore(Builder, LLVMConstInt(LLVMInt8Type(), 0, 1), SndCharPtr);

  LLVMBuildRet(Builder, String);
}

static void addSizeFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "size", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 6);
  strcpy(EscName, "size");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMInt32Type(), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "size", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef StrLenFn = LLVMGetNamedFunction(Module, "strlen");
  LLVMValueRef ParamsStrLen[] = { LLVMGetParam(Function, 0) };
  LLVMValueRef RetValue = LLVMBuildCall(Builder, StrLenFn, ParamsStrLen, 1, "");

  LLVMBuildRet(Builder, RetValue);
}

static void addSubStringFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "substring", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 10);
  strcpy(EscName, "substring");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMInt32Type(),
    LLVMInt32Type()
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), Params, 3, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "substring", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  // len = end - start + 1
  LLVMValueRef SubLength = LLVMBuildSub(Builder, LLVMGetParam(Function, 2), LLVMGetParam(Function, 1), ""); 
  LLVMValueRef TotLength = LLVMBuildAdd(Builder, SubLength, getSConstInt(1), "");
  LLVMValueRef NewString = LLVMBuildArrayMalloc(Builder, LLVMInt8Type(), TotLength, "");

  // *ptr = &string[len]
  LLVMValueRef StartIdx[] = { LLVMGetParam(Function, 1) };
  LLVMValueRef OldAtStart = LLVMBuildInBoundsGEP(Builder, LLVMGetParam(Function, 0), StartIdx, 1, "");

  copyMemory(NewString, OldAtStart, TotLength);

  LLVMBuildRet(Builder, NewString);
}

static void addConcatFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "concat", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 7);
  strcpy(EscName, "concat");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMPointerType(LLVMInt8Type(), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), Params, 2, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "concat", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef StrLenFn = LLVMGetNamedFunction(Module, "strlen");

  // lengthSum = strlen(one) + strlen(two)
  LLVMValueRef ParamsOne[] = { LLVMGetParam(Function, 0) };
  LLVMValueRef LengthOne   = LLVMBuildCall(Builder, StrLenFn, ParamsOne, 1, "");
  LLVMValueRef ParamsTwo[] = { LLVMGetParam(Function, 1) };
  LLVMValueRef LengthTwo   = LLVMBuildCall(Builder, StrLenFn, ParamsTwo, 1, "");

  LLVMValueRef LengthSum_ = LLVMBuildAdd(Builder, LengthOne, LengthTwo, "");
  LLVMValueRef LengthSum  = LLVMBuildAdd(Builder, LengthSum_, getSConstInt(0), "");

  // newString = malloc(lengthSum)
  LLVMValueRef NewString = LLVMBuildArrayMalloc(Builder, LLVMInt8Type(), LengthSum, "");

  copyMemory(NewString, LLVMGetParam(Function, 0), LengthOne);

  LLVMValueRef EndPtr = LLVMBuildInBoundsGEP(Builder, NewString, &LengthOne, 1, "");

  copyMemory(EndPtr, LLVMGetParam(Function, 1), LengthTwo);

  LLVMBuildRet(Builder, NewString);
}

static void addNotFunction(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  char *Name    = pickInsertAlias(ValTable, "not", &toValName, &symTableExists),
       *EscName = (char*) malloc(sizeof(char) * 4);
  strcpy(EscName, "not");

  LLVMTypeRef Params[] = {
    LLVMInt32Type()
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMInt32Type(), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "not", FType);

  LLVMValueRef FnClosure = createClosure(Builder, Function, RA);
  hashInsertOrChange(EV, EscName, FnClosure);
  symTableInsert(ValTable, Name, FnClosure); 

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMBuildRet(Builder, getSConstInt(0));
}


void addAllBaseFunctions(SymbolTable *ValTable, LLVMValueRef RA, Hash *EV) {
  addStdFunctions();
  addLLVMMemCpyFunction();

  LLVMBasicBlockRef MainBB = LLVMGetInsertBlock(Builder);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addPrintFunction    (ValTable, RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addGetCharFunction  (ValTable, RA, EV);
  
  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addFlushFunction    (ValTable, RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addExitFunction     (ValTable, RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addOrdFunction      (ValTable, RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addChrFunction      (ValTable, RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addSizeFunction     (ValTable, RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addSubStringFunction(ValTable, RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addConcatFunction   (ValTable, RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addNotFunction      (ValTable, RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
}

void addAllBaseTypes(SymbolTable *TyTable, LLVMContextRef Context) {
  registerActivationRecord(TyTable, Context);
  registerClosure         (TyTable, Context);
  registerHeap            (TyTable, Context);
}
