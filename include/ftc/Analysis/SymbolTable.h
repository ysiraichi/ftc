#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

typedef struct SymbolTable SymbolTable;

struct SymbolTable {
  Hash Table;
  SymbolTable *Outer;
};

#endif
