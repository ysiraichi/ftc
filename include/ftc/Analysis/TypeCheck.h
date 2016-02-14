#ifndef TYPE_CHECK_H
#define TYPE_CHECK_H

#include "ftc/UtilDeclare.h"
#include "ftc/Analysis/SymbolTable.h"

void semError(int, ASTNode*, const char*, ...);
Type *getRealType(SymbolTable*, Type*);
Type *resolveType(SymbolTable*, Type*);
char *getUnifiedId(SymbolTable*, Type*);
Type *getRecordType(SymbolTable*, Type*);
int typeEqual(SymbolTable*, Type*, Type*);

Type *checkExpr(SymbolTable*, SymbolTable*, ASTNode*);
void checkDecl(SymbolTable*, SymbolTable*, ASTNode*);

#endif
