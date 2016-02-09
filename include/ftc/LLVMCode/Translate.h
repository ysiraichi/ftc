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


LLVMValueRef translateExpr(SymbolTable*, SymbolTable*, ASTNode*);
void         translateDecl(SymbolTable*, SymbolTable*, ASTNode*);

LLVMValueRef toDynamicMemory(LLVMValueRef);
LLVMValueRef getEscapedVar(SymbolTable*, Type*, ASTNode*); 
LLVMValueRef getSConstInt(unsigned long long);

char *pickInsertAlias(SymbolTable*, char*, void (*)(char*,char*), int (*)(SymbolTable*,const char*));
void *resolveAliasId(SymbolTable*, char*, void (*)(char*,char*), void* (*)(SymbolTable*,const char*));
void copyMemory(LLVMValueRef, LLVMValueRef, LLVMValueRef);

int  getLLVMValueTypeKind(LLVMValueRef);
int  getLLVMElementTypeKind(LLVMValueRef);

LLVMTypeRef getLLVMTypeFromType(SymbolTable*, Type*);
LLVMTypeRef toTransitionType(LLVMTypeRef);

#endif
