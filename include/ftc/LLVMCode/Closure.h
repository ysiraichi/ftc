#ifndef CLOSURE_H
#define CLOSURE_H

#include "ftc/Analysis/SymbolTable.h"

#include "llvm-c/Core.h"


void registerClosure(SymbolTable*, LLVMContextRef);

LLVMValueRef createLocalClosure(LLVMBuilderRef, LLVMValueRef, LLVMValueRef);

LLVMValueRef getClosureData(LLVMBuilderRef, LLVMValueRef);
LLVMValueRef getClosureFunction(LLVMBuilderRef, LLVMValueRef);

LLVMValueRef callClosure(LLVMBuilderRef, LLVMTypeRef, LLVMValueRef, LLVMValueRef*, unsigned);

#endif
