#include "ftc/Analysis/BaseEnviroment.h"

static const int CONT = 0;
static const int STR_CONS = 1;

char ReservedWords[][64] = {
  "cont", "stringConsumer", 
  "getchar", "print", "flush", "exit",
  "ord", "chr", "size", "substring", "concat",
  "not"
};

static void addContTy(SymbolTable *St) {
  Type **FunType = (Type**) malloc(sizeof(Type*) * 2);
  FunType[0] = NULL;
  FunType[1] = createType(AnswerTy, NULL);

  symTableInsert(St, ReservedWords[CONT], createType(FunTy, FunType));
}

static void addStrConsumerTy(SymbolTable *St) {
  Type **FunType = (Type**) malloc(sizeof(Type*) * 2);
  FunType[0] = createType(StringTy, NULL);
  FunType[1] = createType(AnswerTy, NULL);

  symTableInsert(St, ReservedWords[STR_CONS], createType(FunTy, FunType));
}

static void addGetChar(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  FunType[0] = createType(IdTy, ReservedWords[STR_CONS]);
  FunType[1] = createType(AnswerTy, NULL);
  symTableInsert(St, ReservedWords[2], createType(FunTy, FunType)); 
}

static void addPrint(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  PtrVector *Vec = createPtrVector();
  ptrVectorAppend(Vec, createType(StringTy, NULL));
  ptrVectorAppend(Vec, createType(IdTy, ReservedWords[CONT]));

  FunType[0] = createType(SeqTy, Vec);
  FunType[1] = createType(AnswerTy, NULL);
  symTableInsert(St, ReservedWords[3], createType(FunTy, FunType)); 
}

static void addFlush(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  FunType[0] = createType(IdTy, ReservedWords[CONT]);
  FunType[1] = createType(AnswerTy, NULL);
  symTableInsert(St, ReservedWords[4], createType(FunTy, FunType)); 
}

static void addExit(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  FunType[0] = NULL;
  FunType[1] = createType(AnswerTy, NULL);
  symTableInsert(St, ReservedWords[5], createType(FunTy, FunType)); 
}

static void addOrd(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  FunType[0] = createType(StringTy, NULL);
  FunType[1] = createType(IntTy, NULL);
  symTableInsert(St, ReservedWords[6], createType(FunTy, FunType)); 
}

static void addChr(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  FunType[0] = createType(IntTy, NULL);
  FunType[1] = createType(StringTy, NULL);
  symTableInsert(St, ReservedWords[7], createType(FunTy, FunType)); 
}

static void addSize(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  FunType[0] = createType(StringTy, NULL);
  FunType[1] = createType(IntTy, NULL);
  symTableInsert(St, ReservedWords[8], createType(FunTy, FunType)); 
}

static void addSubString(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  PtrVector *Vec = createPtrVector();
  ptrVectorAppend(Vec, createType(StringTy, NULL));
  ptrVectorAppend(Vec, createType(IntTy, NULL));
  ptrVectorAppend(Vec, createType(IntTy, NULL));

  FunType[0] = createType(SeqTy, Vec);
  FunType[1] = createType(StringTy, NULL);
  symTableInsert(St, ReservedWords[9], createType(FunTy, FunType)); 
}

static void addConcat(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  PtrVector *Vec = createPtrVector();
  ptrVectorAppend(Vec, createType(StringTy, NULL));
  ptrVectorAppend(Vec, createType(StringTy, NULL));

  FunType[0] = createType(SeqTy, Vec);
  FunType[1] = createType(StringTy, NULL);
  symTableInsert(St, ReservedWords[10], createType(FunTy, FunType)); 
}

static void addNot(SymbolTable *St) {
  Type **FunType  = (Type**) malloc(sizeof(Type*) * 2);

  FunType[0] = createType(IntTy, NULL);
  FunType[1] = createType(IntTy, NULL);
  symTableInsert(St, ReservedWords[11], createType(FunTy, FunType)); 
}

void addBaseEnviroment(SymbolTable *TyTable, SymbolTable *ValTable) {
  addContTy(TyTable);
  addStrConsumerTy(TyTable);

  addGetChar(ValTable);
  addPrint(ValTable);
  addFlush(ValTable);
  addExit(ValTable);
  addOrd(ValTable);
  addChr(ValTable);
  addSize(ValTable);
  addSubString(ValTable);
  addConcat(ValTable);
}
