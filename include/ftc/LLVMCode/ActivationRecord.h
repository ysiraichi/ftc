#ifndef ACTIVATION_RECORD_H
#define ACTIVATION_RECORD_H

#include "ftc/Analysis/SymbolTable.h"

#include "llvm-c/Core.h"


void registerActivationRecord(SymbolTable*, LLVMContextRef);

void putRAHeadAhead(LLVMValueRef);
void returnRAHead();

LLVMValueRef createActivationRecord(LLVMBuilderRef, SymbolTable*, const char*);
LLVMValueRef createCompleteActivationRecord(LLVMBuilderRef, Hash*);
LLVMValueRef createActivationRecordWithSl(LLVMBuilderRef, const char*);
LLVMValueRef createDummyActivationRecord(LLVMBuilderRef);

void createDataLink(LLVMBuilderRef, LLVMValueRef, SymbolTable*, const char*);

#endif
