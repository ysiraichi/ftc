#ifndef HEAP_H
#define HEAP_H

#include "ftc/Analysis/SymbolTable.h"

#include "llvm-c/Core.h"
#include "llvm-c/Target.h"

LLVMValueRef getHeadRA();
LLVMTypeRef  getHeapPointerType();

void registerHeap(SymbolTable*, LLVMContextRef);

void heapPush(LLVMValueRef);
void heapPop ();

#endif
