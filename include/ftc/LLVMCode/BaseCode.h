#ifndef BASE_CODE_H
#define BASE_CODE_H

#include "ftc/Support/Hash.h"
#include "ftc/LLVMCode/Translate.h"
#include "ftc/LLVMCode/Closure.h"
#include "ftc/LLVMCode/ActivationRecord.h"

void addAllBaseFunctions(SymbolTable*, LLVMValueRef, Hash*);
void addAllBaseTypes(SymbolTable*, LLVMContextRef);

#endif
