#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ftc/Support/Hash.h"
#include "ftc/Analysis/Types.h"

typedef struct SymbolTable SymbolTable;

struct SymbolTable {
  Hash Table;
  SymbolTable *Parent;
  PtrVector *Child;
  int IsFunction;
};

void initSymTable(SymbolTable*, int);
SymbolTable *createSymbolTable(SymbolTable*, int);

int symTableInsert(SymbolTable*, char*, void*);
void symTableInsertOrChange(SymbolTable*, char*, void*);

Type *symTableFind(SymbolTable*, char*);
int symTableExists(SymbolTable*, char*);


PtrVectorIterator beginSymTable(SymbolTable*);
PtrVectorIterator endSymTable(SymbolTable*);

#endif
