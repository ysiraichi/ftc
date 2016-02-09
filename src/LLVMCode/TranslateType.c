#include "ftc/LLVMCode/Translate.h"

static LLVMTypeRef getLLVMTypeFromId(SymbolTable *TyTable, Type *Ty) {
  Type *Detailed = (Type*) symTableFind(TyTable, Ty->Val);

  if (Detailed->Kind == RecordTy) {

    return resolveAliasId(TyTable, Ty->Val, &toStructName, &symTableFindGlobal);
  }
  
  if (Detailed->Kind == ArrayTy) {

    return LLVMPointerType(getLLVMTypeFromType(TyTable, Ty->Val), 0);
  }
  return NULL;
}

LLVMTypeRef getLLVMTypeFromType(SymbolTable *TyTable, Type *Ty) {
  if (!Ty) return NULL;
  Type *RealType = getRealType(TyTable, Ty);

  switch (RealType->Kind) {
    case AnswerTy: return LLVMVoidType();
    case IntTy:    return LLVMInt32Type();
    case FloatTy:  return LLVMFloatType();
    case StringTy: return LLVMPointerType(LLVMInt8Type(), 0);

    case FunTy: return symTableFindGlobal(TyTable, "struct.Closure");

    case IdTy:  return getLLVMTypeFromId(TyTable, RealType);

    default: return NULL;
  }
}
