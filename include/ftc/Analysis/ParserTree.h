#ifndef PARSERTREE_H
#define PARSERTREE_H

typedef struct ExprT ExprT;
typedef struct LitT LitT;
typedef struct LvalT LvalT;
typedef struct BinOpT BinOpT;
typedef struct NegT NegT;
typedef struct LetT LetT;
typedef struct DeclT DeclT;
typedef struct TypeT TypeT;
typedef struct ParamTyT ParamTyT;
typedef struct VarDeclT VarDeclT;
typedef struct FunDeclT FunDeclT;
typedef struct ArrayT ArrayT;
typedef struct FunTyT FunTyT;
typedef struct SeqTyT SeqTyT;
typedef struct TyDeclT TyDeclT;
typedef struct IfStmtT IfStmtT;
typedef struct FunCallT FunCallT;
typedef struct ArgT ArgT;
typedef struct CreateT CreateT;
typedef struct ArrayTyT ArrayTyT;
typedef struct RecordT RecordT;
typedef struct FieldT FieldT;

typedef enum { Lit, Lval, BinOp, Let, IfStmt, FunCall, Create } ExprKind;
typedef enum { Int, Float, String, Answer, Cont, StrConsumer, Name, ParamTy, FunTy, ArrayTy } TypeKind;
typedef enum { Id, RecordAccess, ArrayAccess } LvalKind;
typedef enum { Sum, Sub, Mult, Div, Eq, Diff, Lt, Le, Gt, Ge, And, Or } OpKind;
typedef enum { Var, Fun, Ty } DeclKind;
typedef enum { Array, Record } CreateKind;

ExprT    *createExpr(ExprKind, void*);
LitT     *createLit(TypeKind, void*);
LvalT    *createLval(LvalKind, char*, LvalT*, ExprT*);
BinOpT   *createBinOp(OpKind, ExprT*, ExprT*);
NegT     *createNeg(ExprT*);
LetT     *createLet(DeclT*, ExprT*);
DeclT    *createDecl(DeclKind, void*, DeclT*);
TypeT    *createType(TypeKind, void*);
ParamTyT *createParamTy(char*, TypeT*, ParamTyT*);
VarDeclT *createVarDecl(char*, ExprT*);
FunDeclT *createFunDecl(char*, ExprT*, ParamTyT*, FunDeclT *FNext);
TyDeclT  *createTyDecl(char*, TypeT*);
ArrayTyT *createArrayTy(TypeT*);
SeqTyT   *createSeqTy(TypeT*, SeqTyT*);
FunTyT   *createFunTy(SeqTyT*, SeqTyT*);
IfStmtT  *createIfStmt(ExprT*, ExprT*, ExprT*);
ArgT     *createArg(ExprT*, ArgT*);
FunCallT *createFunCall(char*, ArgT*);
CreateT  *createCreate(CreateKind, void*);
ArrayT   *createArray(char*, ExprT*, ExprT*);
RecordT  *createRecord(char*, FieldT*);
FieldT   *createField(char*, ExprT*, FieldT*);

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
  CreateKind Kind;
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
  TypeKind Kind;
  union {
    char     *Id;
    FunTyT   *FunTy;
    ParamTyT *ParamTy;
    ArrayTyT *ArrayTy;
  } Ty;
};

// >> Type Declaration
struct TyDeclT {
  char *Id;
  TypeT *Type;
};

struct SeqTyT {
  TypeT *Type;
  SeqTyT *Next;
};

struct FunTyT {
  SeqTyT *From;
  SeqTyT *To;
};

struct ArrayTyT {
  TypeT *Type;
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
  DeclKind Kind;
  union {
    VarDeclT *Var;
    FunDeclT *Fun;
    TyDeclT  *Ty;
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
  OpKind Kind;
  ExprT *ExprOne;
  ExprT *ExprTwo;
};

struct NegT {
  ExprT *Expr;
};

// -----------------------------------------------= L-Values
struct LvalT {
  LvalKind Kind;
  char *Id;
  LvalT *Lval;
  ExprT *Pos;
};

// -----------------------------------------------= Literals
struct LitT {
  TypeKind Kind;
  union {
    int    Int;
    float  Float;
    char  *String;
  } Value;
};

// -----------------------------------------------= Expression
struct ExprT {
  TypeT *Type;
  ExprKind Kind; 
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
