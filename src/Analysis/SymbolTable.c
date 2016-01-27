#include "ftc/Analysis/SymbolTable.h"

#include <stdio.h>

SymbolTable *createSymbolTable(SymbolTable *Outer) {
  SymbolTable *St = (SymbolTable*) malloc(sizeof(SymbolTable));
  St->Outer = Outer;
  initHash(&(St->Table));
  return St;
}

void symTableInsertOrChange(SymbolTable *St, char *Key, void *Value) {
  hashInsertOrChange(&(St->Table), Key, Value);
}

int symTableInsert(SymbolTable *St, char *Key, void *Value) {
  return hashInsert(&(St->Table), Key, Value);
}

Type *symTableFind(SymbolTable *St, char *Key) {
  if (!St) return NULL;
  void *Value = NULL;
  if (!(Value = hashFind(&(St->Table), Key)))
    return symTableFind(St->Outer, Key);
  return (Type*) Value;
}

int symTableExists(SymbolTable *St, char *Key) {
  if (!St) return 0;
  int Exists = 1;
  if (!(Exists = hashExists(&(St->Table), Key)))
    return symTableExists(St->Outer, Key);
  return Exists;
}

void initSymTable(SymbolTable *St) {
  initHash(&(St->Table));
}

PtrVectorIterator beginSymTable(SymbolTable *St) { return beginHash(&(St->Table)); }
PtrVectorIterator endSymTable(SymbolTable *St) { return endHash(&(St->Table)); }
