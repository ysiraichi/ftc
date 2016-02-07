#ifndef CLOSURE_H
#define CLOSURE_H

#include "ftc/Analysis/SymbolTable.h"

#include "llvm-c/Core.h"


void registerClosure(SymbolTable*, LLVMContextRef);

LLVMValueRef createLocalClosure(LLVMBuilderRef, LLVMValueRef, LLVMValueRef);

#endif
