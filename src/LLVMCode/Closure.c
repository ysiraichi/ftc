#include "ftc/LLVMCode/Closure.h"

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
  LLVMValueRef ClosurePtr = LLVMBuildAlloca(Builder, ClosureTy, "");

  LLVMValueRef DtIdx[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 1),
    LLVMConstInt(LLVMInt32Type(), 0, 1)
  };
  LLVMValueRef Data = LLVMBuildInBoundsGEP(Builder, ClosurePtr, DtIdx, 2, "");
  LLVMBuildStore(Builder, RA, Data);

  LLVMValueRef FnIdx[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 1),
    LLVMConstInt(LLVMInt32Type(), 1, 1)
  };
  LLVMValueRef ClosureFn = LLVMBuildInBoundsGEP(Builder, ClosurePtr, FnIdx, 2, "");
  LLVMValueRef FnIntPtr  = LLVMBuildBitCast(Builder, Fn, LLVMPointerType(LLVMInt8Type(), 0), "");
  LLVMBuildStore(Builder, FnIntPtr, ClosureFn);

  return ClosurePtr;
}
