#include "ftc/LLVMCode/Closure.h"
#include "ftc/LLVMCode/Translate.h"

#include <string.h>

/*
 * Structure type: struct.Closure
 * Attr:
 *   - struct.RA* : Data pointer
 *   - i8*        : Function pointer
 */

static LLVMTypeRef ClosureTy;

void registerClosure(SymbolTable *TyTable, LLVMContextRef Con) {
  char *Name, Buf[] = "struct.Closure", RABuf[] = "struct.RA";
  Name = (char*) malloc(strlen(Buf) * sizeof(char));
  strcpy(Name, Buf);

  LLVMTypeRef RATy = symTableFindGlobal(TyTable, RABuf);

  ClosureTy = LLVMStructCreateNamed(Con, Name);
  symTableInsertGlobal(TyTable, Name, ClosureTy);

  LLVMTypeRef AttrTy[]  = { 
    LLVMPointerType(RATy, 0),
    LLVMPointerType(LLVMInt8Type(), 0)
  };
  LLVMStructSetBody(ClosureTy, AttrTy, 2, 0);
}

LLVMValueRef createLocalClosure(LLVMBuilderRef Builder, LLVMValueRef Fn, LLVMValueRef RA) {
  LLVMValueRef ClosurePtr = LLVMBuildMalloc(Builder, ClosureTy, "LocalClosure");

  LLVMValueRef Data = getClosureData(Builder, ClosurePtr);
  LLVMBuildStore(Builder, RA, Data);

  LLVMValueRef ClosureFn = getClosureFunction(Builder, ClosurePtr);
  LLVMValueRef FnIntPtr  = LLVMBuildBitCast(Builder, Fn, LLVMPointerType(LLVMInt8Type(), 0), "");
  LLVMBuildStore(Builder, FnIntPtr, ClosureFn);

  return ClosurePtr;
}

LLVMValueRef createLocalClosure_(LLVMBuilderRef Builder, LLVMValueRef Fn, LLVMValueRef RA) {
  LLVMValueRef ClosurePtr = LLVMBuildAlloca(Builder, ClosureTy, "LocalClosure");

  LLVMValueRef Data = getClosureData(Builder, ClosurePtr);
  LLVMBuildStore(Builder, RA, Data);

  LLVMValueRef ClosureFn = getClosureFunction(Builder, ClosurePtr);
  LLVMValueRef FnIntPtr  = LLVMBuildBitCast(Builder, Fn, LLVMPointerType(LLVMInt8Type(), 0), "");
  LLVMBuildStore(Builder, FnIntPtr, ClosureFn);

  return ClosurePtr;
}

LLVMValueRef getClosureData(LLVMBuilderRef Builder, LLVMValueRef Closure) {
  LLVMValueRef DtIdx[] = { getSConstInt(0), getSConstInt(0) };
  return LLVMBuildInBoundsGEP(Builder, Closure, DtIdx, 2, "");
}

LLVMValueRef getClosureFunction(LLVMBuilderRef Builder, LLVMValueRef Closure) {
  LLVMValueRef FnIdx[] = { getSConstInt(0), getSConstInt(1) };
  return LLVMBuildInBoundsGEP(Builder, Closure, FnIdx, 2, "");
}

LLVMValueRef callClosure(LLVMBuilderRef Builder, LLVMTypeRef FunctionType, LLVMValueRef Closure, LLVMValueRef *Params, unsigned Count) {
  LLVMValueRef DataRef = getClosureData(Builder, Closure);
  LLVMValueRef FnPtr   = getClosureFunction(Builder, Closure);

  LLVMValueRef FnLoad   = LLVMBuildLoad(Builder, FnPtr, "");
  LLVMValueRef Function = LLVMBuildBitCast(Builder, FnLoad, FunctionType, "CFunction");

  insertNewRA(LLVMBuildLoad(Builder, DataRef, ""));
  LLVMValueRef CallValue = LLVMBuildCall(Builder, Function, Params, Count, "");
  finalizeExecutionRA();

  return CallValue;
}
