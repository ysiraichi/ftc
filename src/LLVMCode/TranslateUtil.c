#include "ftc/LLVMCode/Translate.h"

#include <stdio.h>
#include <string.h>

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

/* Function: Naming conventions. */
void toFunctionName(char *Dst, const char *TypeName) {
  sprintf(Dst, "function.%s", TypeName);
}

void toValName(char *Dst, const char *TypeName) {
  sprintf(Dst, "val.%s", TypeName);
}

void toStructName(char *Dst, const char *TypeName) {
  sprintf(Dst, "struct.%s", TypeName);
}

void toRawName(char *Dst, const char *TypeName) {
  int Count = 0;
  const char *Ptr = TypeName;

  while (*Ptr != '.') ++Ptr;
  ++Ptr;
  while (*Ptr != '.') {
    Dst[Count++] = *Ptr;
    ++Ptr;
  }
  Dst[Count] = '\0';
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

LLVMValueRef getFunctionFromBuilder(LLVMBuilderRef Builder) {
  return LLVMGetBasicBlockParent(LLVMGetInsertBlock(Builder));
}

/* SymbolTable related stuff. */
char *getAliasName(SymbolTable *Table, const char *NodeId, void (*toName)(char*,const char*)) {
  char Buf[NAME_MAX];
  (*toName)(Buf, NodeId);
  return (char*) symTableFind(Table, Buf);
}

LLVMValueRef getAliasFunction(SymbolTable *Table, char *NodeId, void (*toName)(char*,const char*)) {
  char Buf[NAME_MAX], *Name;
  (*toName)(Buf, NodeId);
  Name = (char*) symTableFind(Table, Buf);
  return LLVMGetNamedFunction(Module, Name);
}

void *resolveAliasId(SymbolTable *Table, char *NodeId, void (*toName)(char*,const char*), 
    void *(*tableFind)(SymbolTable*, const char*)) {
  char Buf[NAME_MAX], *Name;
  (*toName)(Buf, NodeId);
  Name = (char*) symTableFind(Table, Buf);
  return (*tableFind)(Table, Name);
}

char *pickInsertAlias(SymbolTable *Table, const char *NodeId, void (*toBaseName)(char*,const char*),
    int (*symbolExists)(SymbolTable*, const char*)) {
  int Count = 0;
  char Buf[NAME_MAX], *BaseName = NULL, *Name = NULL;
  do { 
    (*toBaseName)(Buf, NodeId);
    if (!BaseName) {
      BaseName = (char*) malloc(sizeof(char) * strlen(Buf));
      strcpy(BaseName, Buf);
    } 
    sprintf(Buf, "%s.%d", Buf, Count++);
  } while ((*symbolExists)(Table, Buf));

  Name = (char*) malloc(sizeof(char) * strlen(Buf));
  strcpy(Name, Buf);

  symTableInsert(Table, BaseName, Name);
  return Name;
}

LLVMValueRef getEscapedVar(SymbolTable *ValTable, char *Var, int EscapedLevel) {
  int I, Offset = findEscapedOffset(ValTable, Var, EscapedLevel);
  LLVMValueRef RA        = getHeadRA();
  LLVMValueRef RAContent = LLVMBuildLoad(Builder, RA, "");

  LLVMValueRef StaticLinkIdx[] = { getSConstInt(0), getSConstInt(1) };
  for (I = 0; I < EscapedLevel; ++I) {
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
LLVMValueRef newString(LLVMValueRef From) {
  LLVMValueRef StrLenFn  = LLVMGetNamedFunction(Module, "strlen");
  LLVMValueRef StrLength = LLVMBuildCall(Builder, StrLenFn, &From, 1, "str.length");
  LLVMValueRef NewString = LLVMBuildArrayMalloc(Builder, LLVMInt8Type(), StrLength, "new.string");
  copyMemory(NewString, From, StrLength);
  return NewString;
}

LLVMValueRef toDynamicMemory(LLVMValueRef Val) {
  if (LLVMGetTypeKind(LLVMTypeOf(Val)) != LLVMPointerTypeKind) return Val;

  LLVMTypeRef  ValueType = LLVMGetElementType(LLVMTypeOf(Val));

  if (LLVMGetTypeKind(ValueType) == LLVMStructTypeKind) return Val;

  LLVMValueRef DynMemPtr = NULL;
  LLVMValueRef ValLoad = LLVMBuildLoad(Builder, Val, "ld.static.value");

  switch (LLVMGetTypeKind(ValueType)) {
    case LLVMIntegerTypeKind:
    case LLVMFloatTypeKind:   
      {
        DynMemPtr = LLVMBuildMalloc(Builder, ValueType, "");
        LLVMBuildStore(Builder, ValLoad, DynMemPtr);
        break;
      }
    case LLVMPointerTypeKind:
      {
        DynMemPtr = newString(ValLoad);
        break;
      }
    default: return NULL;
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

LLVMTypeRef wrapStructElementType(LLVMTypeRef Ty) {
  switch (LLVMGetTypeKind(Ty)) {
    case LLVMStructTypeKind: return LLVMPointerType(Ty, 0);

    default: break;
  }
  return Ty;
}

LLVMValueRef wrapValue(LLVMValueRef Val) {
  LLVMTypeRef ValType = LLVMTypeOf(Val);
  switch (LLVMGetTypeKind(ValType)) {
    case LLVMIntegerTypeKind:
    case LLVMFloatTypeKind: 
    case LLVMPointerTypeKind: 
      {
        if (LLVMGetTypeKind(ValType) == LLVMPointerTypeKind &&
            LLVMGetTypeKind(LLVMGetElementType(ValType)) != LLVMIntegerTypeKind)
          break;
        LLVMValueRef Container = LLVMBuildAlloca(Builder, ValType, "wrapped");
        LLVMBuildStore(Builder, Val, Container);
        return Container;
      }
    default: return Val;
  }
  return Val;
}

LLVMValueRef unWrapValue(LLVMValueRef Val) {
  LLVMTypeRef ValType = LLVMGetElementType(LLVMTypeOf(Val));
  switch (LLVMGetTypeKind(ValType)) {
    case LLVMIntegerTypeKind:
    case LLVMFloatTypeKind: 
    case LLVMPointerTypeKind: 
      {
        if (LLVMGetTypeKind(ValType) == LLVMPointerTypeKind &&
            LLVMGetTypeKind(LLVMGetElementType(ValType)) != LLVMIntegerTypeKind)
          break;
        LLVMValueRef ValLoad = LLVMBuildLoad(Builder, Val, "unwrapped");
        return ValLoad;
      }
    default: return Val;
  }
  return Val;
}

/* Transition types (function to function) */
LLVMTypeRef toTransitionType(LLVMTypeRef Ty) {
  switch (LLVMGetTypeKind(Ty)) {
    case LLVMVoidTypeKind:
    case LLVMIntegerTypeKind:
    case LLVMFloatTypeKind: 
    case LLVMPointerTypeKind: return Ty;

    default: return LLVMPointerType(Ty, 0);
  }
}
