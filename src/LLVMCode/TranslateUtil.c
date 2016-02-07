#include "ftc/LLVMCode/Translate.h"

LLVMValueRef getSConstInt(unsigned long long N) {
  return LLVMConstInt(LLVMInt32Type(), N, 1);
}

LLVMValueRef getStructNthValue(LLVMBuilderRef Builder, LLVMValueRef StructPtr, int N) {
  LLVMValueRef Idx[] = { getSConstInt(0), getSConstInt(N) };
  return LLVMBuildInBoundsGEP(Builder, StructPtr, Idx, 2, "");
}
