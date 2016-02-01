#include "ftc/Analysis/SymbolTable.h"

#include <stdio.h>

void initSymTable(SymbolTable *St, int IsFunction) {
  St->IsFunction = IsFunction;

  initHash(&(St->Table));
  initPtrVector(St->Child, 0);
}

SymbolTable *createSymbolTable(SymbolTable *Parent, int IsFunction) {
  SymbolTable *St = (SymbolTable*) malloc(sizeof(SymbolTable));
  initSymTable(St, IsFunction);

  if (Parent) { ptrVectorAppend(Parent->Child, St); }
  St->Parent = Parent;
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
    return symTableFind(St->Parent, Key);
  return (Type*) Value;
}

int symTableExists(SymbolTable *St, char *Key) {
  if (!St) return 0;
  int Exists = 1;
  if (!(Exists = hashExists(&(St->Table), Key)))
    return symTableExists(St->Parent, Key);
  return Exists;
}

PtrVectorIterator beginSymTable(SymbolTable *St) { return beginHash(&(St->Table)); }
PtrVectorIterator endSymTable(SymbolTable *St) { return endHash(&(St->Table)); }
