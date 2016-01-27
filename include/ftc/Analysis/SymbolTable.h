#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ftc/Support/Hash.h"
#include "ftc/Analysis/Types.h"

typedef struct SymbolTable SymbolTable;

struct SymbolTable {
  Hash Table;
  SymbolTable *Outer;
};

SymbolTable *createSymbolTable(SymbolTable *Outer);
int symTableInsert(SymbolTable*, char*, void*);
void symTableInsertOrChange(SymbolTable*, char*, void*);
Type *symTableFind(SymbolTable*, char*);
int symTableExists(SymbolTable*, char*);

void initSymTable(SymbolTable*);
SymbolTable *createSymTable(void);

PtrVectorIterator beginSymTable(SymbolTable*);
PtrVectorIterator endSymTable(SymbolTable*);

#endif
