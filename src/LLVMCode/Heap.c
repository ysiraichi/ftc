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

static LLVMTypeRef  RAType;
static LLVMTypeRef  HeapType;
static LLVMValueRef HeapHead;

static void createPopHeapFunction() {
  // Saving last BasicBlock;
  LLVMBasicBlockRef OldBB = LLVMGetInsertBlock(Builder);

  LLVMTypeRef FunctionType = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);

  LLVMValueRef      Function = LLVMAddFunction(Module, "pop.heap", FunctionType);
  LLVMBasicBlockRef Entry    = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);
  
  // Function Body
  LLVMValueRef HeapHdPtr = LLVMBuildLoad(Builder, HeapHead, "");

  LLVMValueRef LastPtrIdx[] = { getSConstInt(0), getSConstInt(1) };
  LLVMValueRef LastPtr   = LLVMBuildInBoundsGEP(Builder, HeapHdPtr, LastPtrIdx, 2, "heap.last");
  LLVMValueRef LastPtrLd = LLVMBuildLoad(Builder, LastPtr, "ld.heap.last");

  LLVMBuildStore(Builder, LastPtrLd, HeapHead);

  LLVMBuildRetVoid(Builder);

  // Restoring last BasicBlock
  LLVMPositionBuilderAtEnd(Builder, OldBB);
}

static void createPushHeapFunction() {
  // Saving last BasicBlock;
  LLVMBasicBlockRef OldBB = LLVMGetInsertBlock(Builder);

  LLVMTypeRef ParamType    = LLVMPointerType(RAType, 0);
  LLVMTypeRef FunctionType = LLVMFunctionType(LLVMVoidType(), &ParamType, 1, 0);

  LLVMValueRef      Function = LLVMAddFunction(Module, "push.heap", FunctionType);
  LLVMBasicBlockRef Entry    = LLVMAppendBasicBlock(Function, "entry");
  LLVMPositionBuilderAtEnd(Builder, Entry);
  
  // Function Body
  LLVMValueRef HeapMalloc  = LLVMBuildMalloc(Builder, HeapType, "ld.heap.head");

  LLVMValueRef ExPtrIdx[]   = { getSConstInt(0), getSConstInt(0) };
  LLVMValueRef LastPtrIdx[] = { getSConstInt(0), getSConstInt(1) };

  LLVMValueRef ExPtr   = LLVMBuildInBoundsGEP(Builder, HeapMalloc, ExPtrIdx, 2, "heap.exec");
  LLVMValueRef LastPtr = LLVMBuildInBoundsGEP(Builder, HeapMalloc, LastPtrIdx, 2, "heap.last");

  LLVMBuildStore(Builder, LLVMGetParam(Function, 0), ExPtr);
  LLVMBuildStore(Builder, LLVMBuildLoad(Builder, HeapHead, "ld.heap.head"), LastPtr);

  LLVMBuildRetVoid(Builder);

  // Restoring last BasicBlock
  LLVMPositionBuilderAtEnd(Builder, OldBB);
}

/*---------------------------- Compiler related functions.------------------------------*/
LLVMTypeRef getHeapPointerType() {
  return LLVMPointerType(HeapType, 0);
}

LLVMValueRef getHeadRA() {
  LLVMValueRef HeadLd  = LLVMBuildLoad(Builder, HeapHead, "");
  LLVMValueRef RAIdx[] = { getSConstInt(0), getSConstInt(0) };
  return LLVMBuildInBoundsGEP(Builder, HeadLd, RAIdx, 2, "");
}

void registerHeap(SymbolTable *TyTable, LLVMContextRef Con) {
  char *Name, Buf[] = "struct.Heap", BufRA[] = "struct.RA";
  Name = (char*) malloc(strlen(Buf) * sizeof(char));
  strcpy(Name, Buf);

  RAType = symTableFindGlobal(TyTable, BufRA);

  HeapType = LLVMStructCreateNamed(Con, Name);
  symTableInsertGlobal(TyTable, Name, HeapType);

  LLVMTypeRef AttrTy[]  = { 
    LLVMPointerType(RAType, 0), 
    LLVMPointerType(HeapType, 0)
  };  
  LLVMStructSetBody(HeapType, AttrTy, 2, 0); 

  // Initializing Head of Heap.
  HeapHead = LLVMAddGlobal(Module, LLVMPointerType(HeapType, 0), "global.HeapHead");
  LLVMTypeRef HeapHeadConType = LLVMGetElementType(LLVMTypeOf(HeapHead));
  LLVMSetInitializer(HeapHead, LLVMConstPointerNull(HeapHeadConType));

  // Defining functions.
  createPushHeapFunction();
  createPopHeapFunction();
}

void heapPush(LLVMValueRef ExecRA) {
  LLVMValueRef PushFunction = LLVMGetNamedFunction(Module, "push.heap");
  LLVMBuildCall(Builder, PushFunction, &ExecRA, 1, "");
}

void heapPop() {
  LLVMValueRef PopFunction = LLVMGetNamedFunction(Module, "pop.heap");
  LLVMBuildCall(Builder, PopFunction, NULL, 0, "");
}
