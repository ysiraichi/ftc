#include "ftc/LLVMCode/Heap.h"
#include "ftc/LLVMCode/Translate.h"

#include <string.h>

/*
 * Structure type: struct.Heap
 * Attr:
 *   - struct.RA*   : Execution pointer
 *   - struct.Heap* : Last Heap pointer
 */

extern LLVMModuleRef  Module;
extern LLVMBuilderRef Builder;

static LLVMTypeRef  RATy;
static LLVMTypeRef  HeapTy;
static LLVMValueRef Head;

LLVMTypeRef getHeapPointerType() {
  return LLVMPointerType(HeapTy, 0);
}

LLVMValueRef getHeadRA() {
  LLVMValueRef HeadLd  = LLVMBuildLoad(Builder, Head, "");
  LLVMValueRef RAIdx[] = { getSConstInt(0), getSConstInt(0) };
  return LLVMBuildInBoundsGEP(Builder, HeadLd, RAIdx, 2, "");
}

void registerHeap(SymbolTable *TyTable, LLVMContextRef Con) {
  char *Name, Buf[] = "struct.Heap", BufRA[] = "struct.RA";
  Name = (char*) malloc(strlen(Buf) * sizeof(char));
  strcpy(Name, Buf);

  RATy = symTableFindGlobal(TyTable, BufRA);

  HeapTy = LLVMStructCreateNamed(Con, Name);
  symTableInsertGlobal(TyTable, Name, HeapTy);

  LLVMTypeRef AttrTy[]  = { 
    LLVMPointerType(RATy, 0), 
    LLVMPointerType(HeapTy, 0)
  };  
  LLVMStructSetBody(HeapTy, AttrTy, 2, 0); 

  Head = LLVMAddGlobal(Module, LLVMPointerType(HeapTy, 0), "global.Head");
  LLVMSetInitializer(Head, LLVMConstPointerNull(LLVMGetElementType(LLVMTypeOf(Head))));
  LLVMBuildStore(Builder, LLVMConstPointerNull(getHeapPointerType()), Head);
}

void initHeap(LLVMValueRef Heap, LLVMValueRef Ex, LLVMValueRef Lst) {
  LLVMTargetDataRef DataRef = LLVMCreateTargetData(LLVMGetDataLayout(Module));
  unsigned long long Size   = LLVMStoreSizeOfType(DataRef, RATy);

  LLVMValueRef ExMalloc  = LLVMBuildMalloc(Builder, RATy, "");

  LLVMValueRef ExPtrIdx[]   = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef LastPtrIdx[] = { getSConstInt(0), getSConstInt(1) };

  LLVMValueRef ExPtr   = LLVMBuildInBoundsGEP(Builder, Heap, ExPtrIdx, 2, "exec.ra.");
  LLVMValueRef LastPtr = LLVMBuildInBoundsGEP(Builder, Heap, LastPtrIdx, 2, "last.ra.");

  copyMemory(ExMalloc, Ex, getSConstInt(Size));

  LLVMBuildStore(Builder, ExMalloc, ExPtr);
  LLVMBuildStore(Builder, Lst,      LastPtr);
}

void insertNewRA(LLVMValueRef ExecRA) {
  LLVMValueRef New = LLVMBuildMalloc(Builder, HeapTy, "new.ra.");

  LLVMValueRef LastPtr = LLVMBuildLoad(Builder, Head, "loaded.heap.head");

  initHeap(New, ExecRA, LastPtr);
}

void finalizeExecutionRA() {
  LLVMValueRef HeapHdPtr    = LLVMBuildLoad(Builder, Head, "");

  LLVMValueRef LastPtrIdx[] = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef LastPtr   = LLVMBuildInBoundsGEP(Builder, HeapHdPtr, LastPtrIdx, 2, "");
  LLVMValueRef LastPtrLd = LLVMBuildLoad(Builder, LastPtr, "");

  LLVMBuildStore(Builder, LastPtrLd, Head);
}
