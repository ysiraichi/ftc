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

static void createCopyClosureFunction() {
  LLVMBasicBlockRef OldBB = LLVMGetInsertBlock(Builder);

  LLVMTypeRef ParamsType[] = { 
    LLVMPointerType(ClosureType, 0)
  };
  LLVMTypeRef FunctionType = 
    LLVMFunctionType(LLVMPointerType(ClosureType, 0), ParamsType, 1, 0);

  LLVMValueRef Function   = LLVMAddFunction(Module, "copy.closure", FunctionType);
  LLVMBasicBlockRef Entry = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);
  
  // Function Body
  LLVMValueRef OldClosure = LLVMGetParam(Function, 0);
  LLVMValueRef ClosurePtr = LLVMBuildMalloc(Builder, ClosureType, "copy");

  LLVMValueRef OldDataPtr = getClosureData(Builder, OldClosure);
  LLVMValueRef NewDataPtr = getClosureData(Builder, ClosurePtr);
  LLVMValueRef NewData = createDummyActivationRecord(Builder);
  LLVMBuildStore(Builder, NewData, NewDataPtr);

  LLVMValueRef OldData = LLVMBuildLoad(Builder, OldDataPtr, "old.data");

  LLVMValueRef SlIdx[]      = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef SlPointerNew = LLVMBuildInBoundsGEP(Builder, NewData, SlIdx, 2, "new.sl");
  LLVMValueRef SlPointerOld = LLVMBuildInBoundsGEP(Builder, OldData, SlIdx, 2, "old.sl");
  LLVMValueRef SlPointerOldLd = LLVMBuildLoad(Builder, SlPointerOld, "old.sl.ld");
  LLVMBuildStore(Builder, SlPointerOldLd, SlPointerNew);

  LLVMValueRef ClosureFn = getClosureFunction(Builder, ClosurePtr);
  LLVMValueRef OldFnPtr  = getClosureFunction(Builder, OldClosure);
  LLVMValueRef OldFn     = LLVMBuildLoad(Builder, OldFnPtr, "old.fn");
  LLVMBuildStore(Builder, OldFn, ClosureFn);

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
  createCopyClosureFunction();
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
  LLVMValueRef CopyFunction = LLVMGetNamedFunction(Module, "copy.closure");
  LLVMValueRef CopyClos     = LLVMBuildCall(Builder, CopyFunction, &Closure, 1, "copy");

  LLVMValueRef DataPtr = getClosureData(Builder, CopyClos);
  LLVMValueRef DataRef = LLVMBuildLoad(Builder, DataPtr, "");

  LLVMValueRef FnPtr    = getClosureFunction(Builder, CopyClos);
  LLVMValueRef FnLoad   = LLVMBuildLoad(Builder, FnPtr, "");
  LLVMValueRef Function = LLVMBuildBitCast(Builder, FnLoad, FunctionType, "CFunction");

  heapPush(DataRef);
  LLVMValueRef CallValue = LLVMBuildCall(Builder, Function, Params, Count, "");
  heapPop();

  return CallValue;
}
