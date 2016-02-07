#include "ftc/LLVMCode/ActivationRecord.h"
#include "ftc/LLVMCode/Translate.h"

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
  LLVMBuildStore(Builder, LLVMConstPointerNull(getRAPointerType()), RAHead);
}

LLVMValueRef createActivationRecord(LLVMBuilderRef Builder, SymbolTable *St, ASTNode *Node) {
  Hash *EscapedVars = getEscapedVars(St, Node->Value);

  // Malloc the RA struct.
  LLVMValueRef MallocPtr = LLVMBuildMalloc(Builder, RATy, "");

  LLVMValueRef EVIdx[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 1),
    LLVMConstInt(LLVMInt32Type(), 0, 1)
  };
  LLVMValueRef EscVarPtr   = LLVMBuildInBoundsGEP(Builder, MallocPtr, EVIdx, 2, "");

  // Malloc N pointers (escaping variables).
  LLVMValueRef EVMallocPtr = LLVMBuildArrayMalloc(Builder, LLVMPointerType(LLVMInt8Type(), 0), 
      LLVMConstInt(LLVMInt32Type(), EscapedVars->Pairs.Size, 1), "");
  LLVMBuildStore(Builder, EVMallocPtr, EscVarPtr);

  LLVMValueRef SlIdx[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 1),
    LLVMConstInt(LLVMInt32Type(), 1, 1)
  };
  LLVMValueRef StaticLink = LLVMBuildInBoundsGEP(Builder, MallocPtr, SlIdx, 2, "");
  LLVMValueRef HeadLoad   = LLVMBuildLoad(Builder, RAHead, "");
  LLVMBuildStore(Builder, HeadLoad, StaticLink);

  return MallocPtr;
}

void putRAHeadAhead(LLVMValueRef NewRA) {
  LLVMBuildStore(Builder, NewRA, RAHead);
}

void returnRAHead() {
  LLVMValueRef RAHeadLd = LLVMBuildLoad(Builder, RAHead, "");
  LLVMValueRef Idx[]    = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef RANext   = LLVMBuildInBoundsGEP(Builder, RAHeadLd, Idx, 2, "");
  LLVMValueRef RANextLd = LLVMBuildLoad(Builder, RANext, "");
  LLVMBuildStore(Builder, RANextLd, RAHead);
}
