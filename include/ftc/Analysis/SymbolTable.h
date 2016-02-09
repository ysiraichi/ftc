#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ftc/UtilDeclare.h"
#include "ftc/Support/Hash.h"
#include "ftc/Analysis/Types.h"

typedef struct SymbolTable SymbolTable;

struct SymbolTable {
  Hash Table;
  SymbolTable *Parent;
  PtrVector Child;
  ASTNode *Owner;
};

void toEscapedName(char*, char*);

void initSymTable(ASTNode*, SymbolTable*);
SymbolTable *createSymbolTable(ASTNode*, SymbolTable*);

void insertIfEscaped(SymbolTable*, char*);
Hash *getEscapedVars(SymbolTable*, const char*);
void insertIfEscaped(SymbolTable*, char*);
int  findEscapedOffset(SymbolTable*, char*, int);

SymbolTable *symTableFindChild(SymbolTable*, ASTNode*);

int ownerIsFunction(SymbolTable*);
int symTableInsertLocal(SymbolTable*, char*, void*);
int symTableExistsLocal(SymbolTable*, const char*);
void *symTableFindLocal(SymbolTable*, const char*);
int symTableInsertGlobal(SymbolTable*, char*, void*);
int symTableExistsGlobal(SymbolTable*, const char*);
void *symTableFindGlobal(SymbolTable*, const char*);

int symTableInsert(SymbolTable*, char*, void*);
void symTableInsertOrChange(SymbolTable*, char*, void*);

void *symTableFind(SymbolTable*, const char*);
int symTableExists(SymbolTable*, const char*);


PtrVectorIterator beginSymTable(SymbolTable*);
PtrVectorIterator endSymTable(SymbolTable*);

#endif
