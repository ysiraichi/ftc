#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "ftc/Analysis/SymbolTable.h"
#include "ftc/Analysis/TypeCheck.h"

#include "ftc/LLVMCode/Heap.h"
#include "ftc/LLVMCode/ActivationRecord.h"
#include "ftc/LLVMCode/Closure.h"

#include "llvm-c/Core.h"
#include "llvm-c/Target.h"

void toValName     (char*, char*);
void toStructName  (char*, char*);
void toFunctionName(char*, char*);

void        *getAliasValue(SymbolTable*, char*, void (*)(char*,char*), void *(*)(SymbolTable*,char*));
LLVMTypeRef  getLLVMTypeFromType(SymbolTable*, Type*);

LLVMValueRef translateExpr(SymbolTable*, SymbolTable*, ASTNode*);
void         translateDecl(SymbolTable*, SymbolTable*, ASTNode*);

LLVMValueRef toDynamicMemory(LLVMValueRef);
LLVMValueRef getEscapedVar(SymbolTable*, Type*, ASTNode*); 

LLVMValueRef getSConstInt(unsigned long long);
LLVMValueRef getStructNthValue(LLVMBuilderRef, LLVMValueRef, int);

#endif
