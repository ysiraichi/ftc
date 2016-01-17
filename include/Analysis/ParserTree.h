#ifndef PARSERTREE_H
#define PARSERTREE_H

// -----------------------------------------------= Function Call
struct FieldT {
  char   *Id;
  ExprT  *Expr;
  FieldT *Next
};

struct RecordT {
  char   *Id;
  FieldT *Field;
};

struct ArrayT {
  char  *Id;
  ExprT *Size;
  ExprT *Value;
};

struct CreateT {
  int Idx;
  union {
    RecordT *Record;
    ArrayT  *Array;
  } Value;
};

// -----------------------------------------------= Function Call
struct ArgT {
  ExprT *Expr;
  ArgT  *Next;
};

struct FunCallT {
  char *Id;
  ArgT *Args;
};

// -----------------------------------------------= If-Then-Else
struct IfStmtT {
  ExprT *If;
  ExprT *Then;
  ExprT *Else;
};

// -----------------------------------------------= Declarations
// >> Type Declaration
struct TyDeclT {
  char *Id;
  union {
    TypeT  *Type;
    ParamT *Params;
    FunTyT *Func;
    ArrayT *Arry;
  } Ty;
};

struct FunTyT {
  TyDeclT *From;
  TyDeclT *To;
};

struct ArrayT {
  TypeT Type;
};

// >> Function Declaration
struct FunDeclT {
  char   *Id;
  TypeT  *Type;
  ExprT  *Expr;
  ParamT *Params;
  FunDeclT *Next;
};

// >> Variable Declaration
struct VarDeclT {
  char  *Id;
  TypeT *Type;
  ExprT *Expr;
};

// >> Declaration
struct ParamT {
  char  *Id;
  TypeT *Type;
  ParamT *Next;
};

struct TypeT {
  int  Type;
  char *Id;
};

struct DeclT {
  DeclT *Next;
  union {
    VarDeclT *Var;
    FunDeclT *Fun;
    TyDeclT  *Type;
  } Value;
};

// -----------------------------------------------= Let
struct LetT {
  DeclT *Decl;
  ExprT *Expr;
};

// -----------------------------------------------= Binary Operations
struct BinOpT {
  int Idx;
  ExprT *ExprOne;
  ExprT *ExprTwo;
};

// -----------------------------------------------= L-Values
struct LvalT {
  int Idx;
  union {
    char *Id; 
    RecordAccessT *Rec;
    ArrayAccessT  *Array;
  } Value;
};

struct RecordAccessT {
  LvalT *Lval;
  char  *Id;
};

struct ArrayAccessT {
  LvalT *Lval;
  ExprT *Expr;
};

// -----------------------------------------------= Literals
struct LitT {
  int Idx;
  union {
    int    Int;
    float  Flt;
    char  *Str;
  } Value;
};

// -----------------------------------------------= Expression
struct ExprT {
  int Idx;
  union {
    LitT     *Lit;
    LvalT    *Lval;
    BinOpT   *BinOp;
    LetT     *Let;
    IfStmtT  *IfStmt;
    FunCallT *FunCall;
    CreateT  *Create;
  } Value;
};

#endif
