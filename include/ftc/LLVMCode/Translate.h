#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "ftc/Analysis/SymbolTable.h"
#include "ftc/Analysis/TypeCheck.h"

#include "ftc/LLVMCode/Heap.h"
#include "ftc/LLVMCode/ActivationRecord.h"
#include "ftc/LLVMCode/Closure.h"

#include "llvm-c/Core.h"
#include "llvm-c/Target.h"

void toValName     (char*, const char*);
void toStructName  (char*, const char*);
void toFunctionName(char*, const char*);
void toRawName     (char*, const char*);


LLVMValueRef translateExpr(SymbolTable*, SymbolTable*, ASTNode*);
void         translateDecl(SymbolTable*, SymbolTable*, ASTNode*);

LLVMValueRef toDynamicMemory(LLVMValueRef);
LLVMValueRef getEscapedVar(SymbolTable*, char*, int); 
LLVMValueRef getSConstInt(unsigned long long);
LLVMValueRef getAliasFunction(SymbolTable*, char*, void (*)(char*,const char*));
LLVMValueRef wrapValue(LLVMValueRef);
LLVMValueRef unWrapValue(LLVMValueRef);
LLVMValueRef getFunctionFromBuilder(LLVMBuilderRef);

char *getAliasName(SymbolTable*, const char*, void (*)(char*,const char*));
char *pickInsertAlias(SymbolTable*, const char*, void (*)(char*,const char*), int (*)(SymbolTable*,const char*));
void *resolveAliasId(SymbolTable*, char*, void (*)(char*,const char*), void* (*)(SymbolTable*,const char*));
void copyMemory(LLVMValueRef, LLVMValueRef, LLVMValueRef);

int  getLLVMValueTypeKind(LLVMValueRef);
int  getLLVMElementTypeKind(LLVMValueRef);

LLVMTypeRef wrapStructElementType(LLVMTypeRef);
LLVMTypeRef getLLVMTypeFromType(SymbolTable*, Type*);
LLVMTypeRef toTransitionType(LLVMTypeRef);

#endif
