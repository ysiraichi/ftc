#ifndef ACTIVATION_RECORD_H
#define ACTIVATION_RECORD_H

#include "ftc/Analysis/SymbolTable.h"

#include "llvm-c/Core.h"


void registerActivationRecord(SymbolTable*, LLVMContextRef);

void putRAHeadAhead(LLVMValueRef);
void returnRAHead();

LLVMValueRef createActivationRecord(LLVMBuilderRef, SymbolTable*, ASTNode*);

#endif
