#include "ftc/LLVMCode/Translate.h"

#include <stdio.h>
#include <string.h>

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

/* Function: Naming conventions. */
void toFunctionName(char *Dst, char *TypeName) {
  sprintf(Dst, "function.%s", TypeName);
}

void toValName(char *Dst, char *TypeName) {
  sprintf(Dst, "val.%s", TypeName);
}

void toStructName(char *Dst, char *TypeName) {
  sprintf(Dst, "struct.%s", TypeName);
}

/* LLVM library stuff. */
LLVMValueRef getSConstInt(unsigned long long N) {
  return LLVMConstInt(LLVMInt32Type(), N, 1);
}

int getLLVMValueTypeKind(LLVMValueRef Val) {
  return LLVMGetTypeKind(LLVMTypeOf(Val));
}

int getLLVMElementTypeKind(LLVMValueRef Val) {
  return LLVMGetTypeKind(LLVMGetElementType(LLVMTypeOf(Val)));
}

/* SymbolTable related stuff. */
void *resolveAliasId(SymbolTable *Table, char *NodeId, void (*toName)(char*,char*), 
    void *(*tableFind)(SymbolTable*, const char*)) {
  char Buf[NAME_MAX], *Name;
  (*toName)(Buf, NodeId);
  Name = (char*) symTableFind(Table, Buf);
  return (*tableFind)(Table, Name);
}

char *pickInsertAlias(SymbolTable *Table, char *NodeId, void (*toBaseName)(char*,char*),
    int (*symbolExists)(SymbolTable*, const char*)) {
  int Count = 0;
  char Buf[NAME_MAX], *BaseName, *Name;
  do { 
    (*toBaseName)(Buf, NodeId);
    if (!BaseName) {
      BaseName = (char*) malloc(sizeof(char) * strlen(Buf));
      strcpy(BaseName, Buf);
    } 
    sprintf(Buf, "%s%d", Buf, Count++);
  } while ((*symbolExists)(Table, Buf));

  Name = (char*) malloc(sizeof(char) * strlen(Buf));
  strcpy(Name, Buf);

  symTableInsert(Table, BaseName, Name);
  return Name;
}

LLVMValueRef getEscapedVar(SymbolTable *ValTable, Type *VarType, ASTNode *Node) {
  int I, Offset = findEscapedOffset(ValTable, Node->Value, VarType->EscapedLevel);
  LLVMValueRef RA        = getHeadRA();
  LLVMValueRef RAContent = LLVMBuildLoad(Builder, RA, "");

  LLVMValueRef StaticLinkIdx[] = { getSConstInt(0), getSConstInt(1) };
  for (I = 1; I < VarType->EscapedLevel; ++I) {
    RA        = LLVMBuildInBoundsGEP(Builder, RAContent, StaticLinkIdx, 2, "");
    RAContent = LLVMBuildLoad(Builder, RA, "");
  }

  LLVMValueRef EscapedVarsIdx[] = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef EscapedVarsPtr   = LLVMBuildInBoundsGEP(Builder, RAContent, EscapedVarsIdx, 2, "");
  LLVMValueRef EscapedVars      = LLVMBuildLoad(Builder, EscapedVarsPtr, "");

  LLVMValueRef VarOffsetIdx[] = { getSConstInt(Offset) };
  LLVMValueRef EscapedVar     = LLVMBuildInBoundsGEP(Builder, EscapedVars, VarOffsetIdx, 1, "");
  /*
   * Return a <type>**
   */
  return EscapedVar;
}

/* Memory functions. */
LLVMValueRef toDynamicMemory(LLVMValueRef Val) {
  LLVMTypeRef  ValueType = LLVMGetElementType(LLVMTypeOf(Val));

  LLVMValueRef DynMemPtr = LLVMBuildMalloc(Builder, ValueType, "");

  LLVMTargetDataRef DataRef = LLVMCreateTargetData(LLVMGetDataLayout(Module));
  unsigned long long Size   = LLVMStoreSizeOfType(DataRef, ValueType);

  copyMemory(DynMemPtr, Val, getSConstInt(Size));
  if (LLVMGetTypeKind(ValueType) == LLVMPointerTypeKind) {
    LLVMValueRef PointeeMalloc = toDynamicMemory(LLVMBuildLoad(Builder, Val, ""));
    LLVMBuildStore(Builder, PointeeMalloc, DynMemPtr);
  }
  return DynMemPtr;
}

void copyMemory(LLVMValueRef To, LLVMValueRef From, LLVMValueRef Length) {
  LLVMValueRef MemCpyFn = LLVMGetNamedFunction(Module, "llvm.memcpy.p0i8.p0i8.i32");

  LLVMValueRef ToI8   = LLVMBuildBitCast(Builder, To,   LLVMPointerType(LLVMInt8Type(), 0), "");
  LLVMValueRef FromI8 = LLVMBuildBitCast(Builder, From, LLVMPointerType(LLVMInt8Type(), 0), "");

  LLVMValueRef Params[] = {
    ToI8,
    FromI8,
    Length,
    getSConstInt(1),
    LLVMConstInt(LLVMIntType(1), 0, 0)
  };

  LLVMBuildCall(Builder, MemCpyFn, Params, 5, "");
}

/* Transition types (function to function) */
LLVMTypeRef toTransitinType(LLVMTypeRef Ty) {
  switch (LLVMGetTypeKind(Ty)) {
    case LLVMIntegerTypeKind:
    case LLVMFloatTypeKind: 
    case LLVMPointerTypeKind: return Ty;

    default: return LLVMPointerType(Ty, 0);
  }
}
