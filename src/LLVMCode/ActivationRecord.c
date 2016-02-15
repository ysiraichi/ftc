#include "ftc/LLVMCode/ActivationRecord.h"
#include "ftc/LLVMCode/Translate.h"

#include <stdio.h>
#include <string.h>

/*
 * Structure type: struct.RA
 * Attr:
 *   - i8**       : Pointer to escaping variables
 *   - struct.Ra* : Static link
 */

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

static LLVMTypeRef  RATy;
static LLVMValueRef RAHead;

LLVMTypeRef getRAPointerType() {
  return LLVMPointerType(RATy, 0);
}

void registerActivationRecord(SymbolTable *TyTable, LLVMContextRef Con) {
  char *Name, Buf[] = "struct.RA";
  Name = (char*) malloc(strlen(Buf) * sizeof(char));
  strcpy(Name, Buf);

  RATy = LLVMStructCreateNamed(Con, Name);
  symTableInsertGlobal(TyTable, Name, RATy);

  LLVMTypeRef AttrTy[] = { 
    LLVMPointerType(LLVMPointerType(LLVMInt8Type(), 0), 0),
    LLVMPointerType(RATy, 0)
  };
  LLVMStructSetBody(RATy, AttrTy, 2, 0);

  RAHead = LLVMAddGlobal(Module, getRAPointerType(), "global.RAHead");
  LLVMSetInitializer(RAHead, LLVMConstPointerNull(LLVMGetElementType(LLVMTypeOf(RAHead))));
  LLVMBuildStore(Builder, LLVMConstPointerNull(getRAPointerType()), RAHead);
}

void putRAHeadAhead(LLVMValueRef NewRA) {
  LLVMBuildStore(Builder, NewRA, RAHead);
}

void returnRAHead() {
  LLVMValueRef RAHeadLd = LLVMBuildLoad(Builder, RAHead, "return.load");
  LLVMValueRef Idx[]    = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef RANext   = LLVMBuildInBoundsGEP(Builder, RAHeadLd, Idx, 2, "");
  LLVMValueRef RANextLd = LLVMBuildLoad(Builder, RANext, "return.ptr");
  LLVMBuildStore(Builder, RANextLd, RAHead);
}

LLVMValueRef createActivationRecord(LLVMBuilderRef Builder, SymbolTable *St, const char *FName) {
  char Buf[NAME_MAX];
  sprintf(Buf, "createRA.for.%s", FName);
  Hash *EscapedVars = getEscapedVars(St, FName);

  // Malloc the RA struct.
  LLVMValueRef MallocPtr = LLVMBuildMalloc(Builder, RATy, Buf);

  LLVMValueRef EVIdx[]   = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef EscVarPtr   = LLVMBuildInBoundsGEP(Builder, MallocPtr, EVIdx, 2, "");

  // Malloc N pointers (escaping variables).
  LLVMValueRef EVMallocPtr = LLVMBuildArrayMalloc(Builder, LLVMPointerType(LLVMInt8Type(), 0), 
      LLVMConstInt(LLVMInt32Type(), EscapedVars->Pairs.Size, 1), "");
  LLVMBuildStore(Builder, EVMallocPtr, EscVarPtr);

  LLVMValueRef SlIdx[] = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef StaticLink = LLVMBuildInBoundsGEP(Builder, MallocPtr, SlIdx, 2, "");
  LLVMValueRef HeadLoad   = LLVMBuildLoad(Builder, RAHead, "");
  LLVMBuildStore(Builder, HeadLoad, StaticLink);

  return MallocPtr;
}

LLVMValueRef createActivationRecordWithSl(LLVMBuilderRef Builder, const char *FName) {
  char Buf[NAME_MAX];
  sprintf(Buf, "createRA.sl.for.%s", FName);

  // Malloc the RA struct.
  LLVMValueRef MallocPtr = LLVMBuildMalloc(Builder, RATy, Buf);

  LLVMValueRef EVIdx[]   = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef EscVarPtr = LLVMBuildInBoundsGEP(Builder, MallocPtr, EVIdx, 2, "");
  LLVMBuildStore(Builder, LLVMConstPointerNull(LLVMGetElementType(LLVMTypeOf(EscVarPtr))), EscVarPtr);

  LLVMValueRef SlIdx[] = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef StaticLink = LLVMBuildInBoundsGEP(Builder, MallocPtr, SlIdx, 2, "");
  LLVMValueRef HeadLoad   = LLVMBuildLoad(Builder, RAHead, "");
  LLVMBuildStore(Builder, HeadLoad, StaticLink);

  return MallocPtr;
}

LLVMValueRef createCompleteActivationRecord(LLVMBuilderRef Builder, Hash *EscapedVars) {
  // Malloc the RA struct.
  LLVMValueRef MallocPtr = LLVMBuildMalloc(Builder, RATy, "");

  LLVMValueRef EVIdx[]   = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef EscVarPtr   = LLVMBuildInBoundsGEP(Builder, MallocPtr, EVIdx, 2, "");

  // Malloc N pointers (escaping variables).
  LLVMValueRef EVMallocPtr = LLVMBuildArrayMalloc(Builder, LLVMPointerType(LLVMInt8Type(), 0), 
      getSConstInt(EscapedVars->Pairs.Size), "");
  LLVMBuildStore(Builder, EVMallocPtr, EscVarPtr);

  unsigned Count;
  for (Count = 0; Count < EscapedVars->Pairs.Size; ++Count) {
    Pair *P = (Pair*) ptrVectorGet(&(EscapedVars->Pairs), Count);
    LLVMValueRef ClosurePtr = (LLVMValueRef) P->second;
    if (!ClosurePtr) {
      continue;
    }

    LLVMValueRef ElemIdx[] = { getSConstInt(Count) };
    LLVMValueRef ElemPtr   = LLVMBuildInBoundsGEP(Builder, EVMallocPtr, ElemIdx, 1, "");
    LLVMValueRef ClosureI8 = LLVMBuildBitCast(Builder, ClosurePtr, LLVMPointerType(LLVMInt8Type(), 0), "");
    LLVMBuildStore(Builder, ClosureI8, ElemPtr);
  }

  LLVMValueRef SlIdx[] = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef StaticLink = LLVMBuildInBoundsGEP(Builder, MallocPtr, SlIdx, 2, "");
  LLVMValueRef HeadLoad   = LLVMBuildLoad(Builder, RAHead, "");
  LLVMBuildStore(Builder, HeadLoad, StaticLink);

  return MallocPtr;
}

void createDataLink(LLVMBuilderRef Builder, LLVMValueRef RA, SymbolTable *St, const char *FName) {
  Hash *EscapedVars = getEscapedVars(St, FName);
  LLVMValueRef EVIdx[]   = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef EscVarPtr   = LLVMBuildInBoundsGEP(Builder, RA, EVIdx, 2, "data.link.ptr");

  LLVMValueRef EVMallocPtr = LLVMBuildArrayMalloc(Builder, LLVMPointerType(LLVMInt8Type(), 0), 
      LLVMConstInt(LLVMInt32Type(), EscapedVars->Pairs.Size, 1), "new.data.link");
  LLVMBuildStore(Builder, EVMallocPtr, EscVarPtr);
}

LLVMValueRef createDummyActivationRecord(LLVMBuilderRef Builder) {
  // Malloc the RA struct.
  LLVMValueRef MallocPtr = LLVMBuildMalloc(Builder, RATy, "");

  LLVMValueRef EVIdx[]   = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef EscVarPtr = LLVMBuildInBoundsGEP(Builder, MallocPtr, EVIdx, 2, "");
  LLVMBuildStore(Builder, LLVMConstPointerNull(LLVMGetElementType(LLVMTypeOf(EscVarPtr))), EscVarPtr);

  LLVMValueRef SlIdx[] = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef StaticLink = LLVMBuildInBoundsGEP(Builder, MallocPtr, SlIdx, 2, "");
  LLVMBuildStore(Builder, LLVMConstPointerNull(LLVMGetElementType(LLVMTypeOf(StaticLink))), StaticLink);

  return MallocPtr;
}
