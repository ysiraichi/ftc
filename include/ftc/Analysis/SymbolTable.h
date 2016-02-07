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
Hash *getEscapedVars(SymbolTable*, char*);
void insertIfEscaped(SymbolTable*, char*);
int  findEscapedOffset(SymbolTable*, char*, int);

SymbolTable *symTableFindChild(SymbolTable*, ASTNode*);

int ownerIsFunction(SymbolTable*);
int symTableInsertLocal(SymbolTable*, char*, void*);
int symTableExistsLocal(SymbolTable*, char*);
void *symTableFindLocal(SymbolTable*, char*);
int symTableInsertGlobal(SymbolTable*, char*, void*);
int symTableExistsGlobal(SymbolTable*, char*);
void *symTableFindGlobal(SymbolTable*, char*);

int symTableInsert(SymbolTable*, char*, void*);
void symTableInsertOrChange(SymbolTable*, char*, void*);

void *symTableFind(SymbolTable*, char*);
int symTableExists(SymbolTable*, char*);


PtrVectorIterator beginSymTable(SymbolTable*);
PtrVectorIterator endSymTable(SymbolTable*);

#endif
