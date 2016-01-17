#ifndef PARSERTREE_H
#define PARSERTREE_H

typedef struct ExprT ExprT;
typedef struct LitT LitT;
typedef struct ArrayAccessT ArrayAccessT;
typedef struct RecordAccessT RecordAccessT;
typedef struct LvalT LvalT;
typedef struct BinOpT BinOpT;
typedef struct LetT LetT;
typedef struct DeclT DeclT;
typedef struct TypeT TypeT;
typedef struct ParamTyT ParamTyT;
typedef struct VarDeclT VarDeclT;
typedef struct FunDeclT FunDeclT;
typedef struct ArrayT ArrayT;
typedef struct FunTyT FunTyT;
typedef struct TyDeclT TyDeclT;
typedef struct IfStmtT IfStmtT;
typedef struct FunCallT FunCallT;
typedef struct ArgT ArgT;
typedef struct CreateT CreateT;
typedef struct ArrayTyT ArrayTyT;
typedef struct RecordT RecordT;
typedef struct FieldT FieldT;

// -----------------------------------------------= Function Call
struct FieldT {
  char   *Id;
  ExprT  *Expr;
  FieldT *Next;
  TypeT *Type;
};

struct RecordT {
  char   *Id;
  FieldT *Field;
  TypeT *Type;
};

struct ArrayT {
  char  *Id;
  ExprT *Size;
  ExprT *Value;
  TypeT *Type;
};

struct CreateT {
  enum {
    Record, Array 
  } Idx;
  union {
    RecordT *Record;
    ArrayT  *Array;
  } Value;
  TypeT *Type;
};

// -----------------------------------------------= Function Call
struct ArgT {
  ExprT *Expr;
  ArgT  *Next;
  TypeT *Type;
};

struct FunCallT {
  char *Id;
  ArgT *Args;
  TypeT *Type;
};

// -----------------------------------------------= If-Then-Else
struct IfStmtT {
  TypeT *Type;
  ExprT *If;
  ExprT *Then;
  ExprT *Else;
};

// -----------------------------------------------= Declarations
struct ParamTyT {
  char  *Id;
  TypeT *Type;
  ParamTyT *Next;
};

struct TypeT {
  enum { 
    Preset, Custom 
  } Idx;
  union {
    int  Type;
    char *Id;
  } Value;
};

// >> Type Declaration
struct TyDeclT {
  char *Id;
  enum {
    Type, ParamTy, FunTy, ArrayTy
  } Idx;
  union {
    TypeT    *Type;
    ParamTyT   *Params;
    FunTyT   *Func;
    ArrayTyT *ArrayTy;
  } Ty;
};

struct FunTyT {
  TyDeclT *From;
  TyDeclT *To;
};

struct ArrayTyT {
  TypeT Type;
};

// >> Function Declaration
struct FunDeclT {
  char   *Id;
  TypeT  *Type;
  ExprT  *Expr;
  ParamTyT *Params;
  FunDeclT *Next;
};

// >> Variable Declaration
struct VarDeclT {
  char  *Id;
  TypeT *Type;
  ExprT *Expr;
};

// >> Declaration
struct DeclT {
  enum {
    Var, Fun, Ty
  } Idx;
  union {
    VarDeclT *Var;
    FunDeclT *Fun;
    TyDeclT  *Type;
  } Value;
  DeclT *Next;
};

// -----------------------------------------------= Let
struct LetT {
  TypeT *Type;
  DeclT *Decl;
  ExprT *Expr;
};

// -----------------------------------------------= Binary Operations
struct BinOpT {
  TypeT *Type;
  enum {
    Sum, Sub, Mult, Div
  } Idx;
  ExprT *ExprOne;
  ExprT *ExprTwo;
};

// -----------------------------------------------= L-Values
struct LvalT {
  enum {
    Id, RecordAccess, ArrayAccess
  } Idx;
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
  enum { 
    Int, Float, String
  } Idx;
  union {
    int    Int;
    float  Flt;
    char  *Str;
  } Value;
};

// -----------------------------------------------= Expression
struct ExprT {
  TypeT *Type;
  enum {
    Lit, Lval, BinOp, Let, IfStmt, FunCall, Create
  } Idx;
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
