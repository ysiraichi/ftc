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

static void addPrintFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 6);
  strcpy(Name, "print");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMPointerType(LLVMGetTypeByName(Module, "struct.Closure"), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMVoidType(), Params, 2, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "print", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef PrintfFn   = LLVMGetNamedFunction(Module, "printf");
  LLVMValueRef Args[]     = { LLVMGetParam(Function, 0) };
  LLVMBuildCall(Builder, PrintfFn, Args, 1, "");

  LLVMValueRef ContFn  = LLVMGetParam(Function, 1);
  callClosure(Builder, getContFunctionPtrType(), ContFn, NULL, 0); 

  LLVMBuildRetVoid(Builder);
}

static void addGetCharFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 8);
  strcpy(Name, "getchar");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMGetTypeByName(Module, "struct.Closure"), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "getcharTiger", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

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
  callClosure(Builder, getStringConsumerFunctionPtrType(), Closure, &String, 1);

  LLVMBuildRet(Builder, String);
}

static void addFlushFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 6);
  strcpy(Name, "flush");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMGetTypeByName(Module, "struct.Closure"), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMVoidType(), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "flush", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef FlushFn   = LLVMGetNamedFunction(Module, "fflush");
  LLVMValueRef GlbStdout = LLVMGetNamedGlobal(Module, "stdout");
  LLVMValueRef GlbLoad   = LLVMBuildLoad(Builder, GlbStdout, "");
  LLVMBuildCall(Builder, FlushFn, &GlbLoad, 1, "");

  LLVMValueRef Closure = LLVMGetParam(Function, 0);
  callClosure(Builder, getContFunctionPtrType(), Closure, NULL, 0);

  LLVMBuildRetVoid(Builder);
}

static void addExitFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 5);
  strcpy(Name, "exit");

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "exitTiger", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef ExitFn   = LLVMGetNamedFunction(Module, "exit");
  LLVMValueRef Params[]  = { getSConstInt(0) };
  LLVMBuildCall(Builder, ExitFn, Params, 1, "");

  LLVMBuildRetVoid(Builder);
}

static void addOrdFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 4);
  strcpy(Name, "ord");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMInt32Type(), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "ord", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef FstChar  = LLVMBuildLoad(Builder, LLVMGetParam(Function, 0), "");
  LLVMValueRef RetValue = LLVMBuildSExt(Builder, FstChar, LLVMInt32Type(), "");

  LLVMBuildRet(Builder, RetValue);
}

static void addChrFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 4);
  strcpy(Name, "chr");

  LLVMTypeRef Params[] = {
    LLVMInt32Type()
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "chr", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

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

static void addSizeFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 5);
  strcpy(Name, "size");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMInt32Type(), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "size", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMValueRef StrLenFn = LLVMGetNamedFunction(Module, "strlen");
  LLVMValueRef ParamsStrLen[] = { LLVMGetParam(Function, 0) };
  LLVMValueRef RetValue = LLVMBuildCall(Builder, StrLenFn, ParamsStrLen, 1, "");

  LLVMBuildRet(Builder, RetValue);
}

static void addSubStringFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 10);
  strcpy(Name, "substring");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMInt32Type(),
    LLVMInt32Type()
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), Params, 3, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "substring", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

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

static void addConcatFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 7);
  strcpy(Name, "concat");

  LLVMTypeRef Params[] = {
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMPointerType(LLVMInt8Type(), 0)
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), Params, 2, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "concat", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

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

static void addNotFunction(LLVMValueRef RA, Hash *EV) {
  char *Name = (char*) malloc(sizeof(char) * 4);
  strcpy(Name, "not");

  LLVMTypeRef Params[] = {
    LLVMInt32Type()
  };

  LLVMTypeRef  FType    = LLVMFunctionType(LLVMInt32Type(), Params, 1, 0);
  LLVMValueRef Function = LLVMAddFunction(Module, "not", FType);
  hashInsert(EV, Name, createLocalClosure(Builder, Function, RA));

  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);

  LLVMBuildRet(Builder, getSConstInt(0));
}


void addAllBaseFunctions(LLVMValueRef RA, Hash *EV) {
  addStdFunctions();
  addLLVMMemCpyFunction();

  LLVMBasicBlockRef MainBB = LLVMGetInsertBlock(Builder);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addPrintFunction    (RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addGetCharFunction  (RA, EV);
  
  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addFlushFunction    (RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addExitFunction     (RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addOrdFunction      (RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addChrFunction      (RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addSizeFunction     (RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addSubStringFunction(RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addConcatFunction   (RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
  addNotFunction      (RA, EV);

  LLVMPositionBuilderAtEnd(Builder, MainBB);
}

void addAllBaseTypes(SymbolTable *TyTable, LLVMContextRef Context) {
  registerActivationRecord(TyTable, Context);
  registerClosure         (TyTable, Context);
  registerHeap            (TyTable, Context);
}
