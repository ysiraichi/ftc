#include "ftc/LLVMCode/Closure.h"
#include "ftc/LLVMCode/Translate.h"

#include <string.h>

/*
 * Structure type: struct.Closure
 * Attr:
 *   - struct.RA* : Data pointer
 *   - i8*        : Function pointer
 */

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

static LLVMTypeRef ClosureType;
static LLVMTypeRef RAType;

static void createCreateClosureFunction() {
  LLVMBasicBlockRef OldBB = LLVMGetInsertBlock(Builder);

  LLVMTypeRef ParamsType[] = { 
    LLVMPointerType(RAType, 0),
    LLVMPointerType(LLVMInt8Type(), 0)
  };
  LLVMTypeRef FunctionType = 
    LLVMFunctionType(LLVMPointerType(ClosureType, 0), ParamsType, 2, 0);

  LLVMValueRef Function   = LLVMAddFunction(Module, "create.closure", FunctionType);
  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);
  
  // Function Body
  LLVMValueRef ClosurePtr = LLVMBuildMalloc(Builder, ClosureType, "closure");

  LLVMValueRef Data = getClosureData(Builder, ClosurePtr);
  LLVMBuildStore(Builder, LLVMGetParam(Function, 0), Data);

  LLVMValueRef ClosureFn = getClosureFunction(Builder, ClosurePtr);
  LLVMBuildStore(Builder, LLVMGetParam(Function, 1), ClosureFn);

  LLVMBuildRet(Builder, ClosurePtr);

  LLVMPositionBuilderAtEnd(Builder, OldBB);
}

void registerClosure(SymbolTable *TyTable, LLVMContextRef Con) {
  char *Name, Buf[] = "struct.Closure", RABuf[] = "struct.RA";
  Name = (char*) malloc(strlen(Buf) * sizeof(char));
  strcpy(Name, Buf);

  RAType = symTableFindGlobal(TyTable, RABuf);

  ClosureType = LLVMStructCreateNamed(Con, Name);
  symTableInsertGlobal(TyTable, Name, ClosureType);

  LLVMTypeRef AttrTy[]  = { 
    LLVMPointerType(RAType, 0),
    LLVMPointerType(LLVMInt8Type(), 0)
  };
  LLVMStructSetBody(ClosureType, AttrTy, 2, 0);

  // Functions
  createCreateClosureFunction();
}

LLVMValueRef createClosure(LLVMBuilderRef Builder, LLVMValueRef Fn, LLVMValueRef RA) {
  LLVMValueRef CClosureFunction = LLVMGetNamedFunction(Module, "create.closure");

  LLVMValueRef FnIntPtr    = LLVMBuildBitCast(Builder, Fn, LLVMPointerType(LLVMInt8Type(), 0), "");
  LLVMValueRef ArgsValue[] = { RA, FnIntPtr };
  return LLVMBuildCall(Builder, CClosureFunction, ArgsValue, 2, "created.closure");
}

LLVMValueRef getClosureData(LLVMBuilderRef Builder, LLVMValueRef Closure) {
  LLVMValueRef DtIdx[] = { getSConstInt(0), getSConstInt(0) };
  return LLVMBuildInBoundsGEP(Builder, Closure, DtIdx, 2, "");
}

LLVMValueRef getClosureFunction(LLVMBuilderRef Builder, LLVMValueRef Closure) {
  LLVMValueRef FnIdx[] = { getSConstInt(0), getSConstInt(1) };
  return LLVMBuildInBoundsGEP(Builder, Closure, FnIdx, 2, "");
}

LLVMValueRef callClosure(LLVMTypeRef FunctionType, LLVMValueRef Closure, LLVMValueRef *Params, unsigned Count) {
  LLVMValueRef DataRef = getClosureData(Builder, Closure);
  LLVMValueRef FnPtr   = getClosureFunction(Builder, Closure);

  LLVMValueRef FnLoad   = LLVMBuildLoad(Builder, FnPtr, "");
  LLVMValueRef Function = LLVMBuildBitCast(Builder, FnLoad, FunctionType, "CFunction");

  heapPush(LLVMBuildLoad(Builder, DataRef, ""));
  LLVMValueRef CallValue = LLVMBuildCall(Builder, Function, Params, Count, "");
  heapPop();

  return CallValue;
}
