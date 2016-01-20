%{
#include <stdio.h>
#include <stdarg.h>

#include "ftc/Analysis/ParserTree.h"

int yylex(void);

static const char LexError[] = "LexicalError";
static const char SynError[] = "SyntaticalError";

ExprT *Root;

%}

%code requires {
  void yyerror(const char*, ...);
}

%locations
%define parse.error verbose

%union {
  int Int;
  float Float;
  char *String;
  void *Node;
}

%token <Int> INT 
%token <Float> FLT 
%token <String> STR ID

%token LET IN END FUN VAR IF THEN ELSE NIL ARRY OF TYPE
%token PLS MIN DIV MUL ASGN EQ DIF LT LE GT GE AND OR

%token LPAR RPAR LSQB RSQB LBRK RBRK COLL COMM PT SMCL

%token INTO
%token INTT FLTT STRT ANST CNTT STRCT

%precedence THEN
%precedence ELSE

%right INTO


%precedence OF

%precedence LVAL
%precedence LSQB

%precedence "declaration"
%precedence FUN

%left OR
%left AND
%left EQ DIF LT LE GT GE
%left PLS MIN
%left DIV MUL
%right UMIN

%type <Node> program

%type <Node> expr lit l-val arithm unary compare logic let if-then fun-call par-expr create

%type <Node> record array decls decl var-decl fun-decl ty-decl id-type arg-decl
%type <Node> fun-ty-decl ty-seq fun-decls arg-expr rec-create arr-create rec-field

%type <Node> arg-decl2 var-decl2 ty-decl2 ty-seq2 fun-decl2 arg-expr2 rec-field2

%start program

%%

program: expr                { $$ = $1; Root = $$; }
       | /* empty */         { $$ = NULL; }

expr: lit                    { $$ = (void*) createExpr(Lit, $1); }
    | l-val                  { $$ = (void*) createExpr(Lval, $1); }
    | arithm                 { $$ = (void*) createExpr(BinOp, $1); }
    | compare                { $$ = (void*) createExpr(BinOp, $1); }
    | logic                  { $$ = (void*) createExpr(BinOp, $1); }
    | unary                  { $$ = (void*) createExpr(Neg, $1); }
    | let                    { $$ = (void*) createExpr(Let, $1); }
    | if-then                { $$ = (void*) createExpr(IfStmt, $1); }
    | fun-call               { $$ = (void*) createExpr(FunCall, $1); }
    | create                 { $$ = (void*) createExpr(Create, $1); }
    | par-expr               { $$ = $1; }
    | NIL                    { $$ = (void*) createExpr(Nil, createNil()); }

/* ----------- primitive types --------------- */

lit: INT                     { $$ = (void*) createLit(Int, &($1)); }
   | FLT                     { $$ = (void*) createLit(Float, &($1)); }
   | STR                     { $$ = (void*) createLit(String, $1); }

/* ----------------- l-value ----------------- */

l-val: record                { $$ = $1; }
     | array                 { $$ = $1; }
     | ID %prec LVAL         { $$ = (void*) createLval(Id, $1, NULL, NULL); }

record: l-val PT ID           { $$ = (void*) createLval(RecordAccess, $3, $1, NULL); }

array: l-val LSQB expr RSQB   { $$ = (void*) createLval(ArrayAccess, NULL, $1, $3); }

/* ---------------- arithmetic --------------- */

arithm: expr PLS expr        { $$ = (void*) createBinOp(Sum, $1, $3); }
      | expr MIN expr        { $$ = (void*) createBinOp(Sub, $1, $3); }
      | expr MUL expr        { $$ = (void*) createBinOp(Mult, $1, $3); }
      | expr DIV expr        { $$ = (void*) createBinOp(Div, $1, $3); }

unary: MIN expr %prec UMIN   { $$ = (void*) createNeg($2); }

/* --------------- comparison ---------------- */

compare: expr EQ expr        { $$ = (void*) createBinOp(Eq, $1, $3); }
       | expr DIF expr       { $$ = (void*) createBinOp(Diff, $1, $3); } 
       | expr LT expr        { $$ = (void*) createBinOp(Lt, $1, $3); }
       | expr LE expr        { $$ = (void*) createBinOp(Le, $1, $3); }
       | expr GT expr        { $$ = (void*) createBinOp(Gt, $1, $3); }
       | expr GE expr        { $$ = (void*) createBinOp(Ge, $1, $3); }

/* ------------------ logic ------------------ */

logic: expr AND expr        { $$ = (void*) createBinOp(And, $1, $3); }
     | expr OR expr         { $$ = (void*) createBinOp(Or, $1, $3); }

/* ------------------ let -------------------- */

let: LET decls IN expr END  { $$ = (void*) createLet($2, $4); }

/* --------------- declarations -------------- */

decls: decls decl           { 
                              if ($1) { 
                                DeclT *Ptr = (DeclT*) $1;
                                while (Ptr->Next) Ptr = Ptr->Next;
                                Ptr->Next = $2; 
                                $$ = $1; 
                              } else {
                                $$ = $2; 
                              }
                            }
     | /* empty */          { $$ = NULL; }

decl: var-decl                        { $$ = (void*) createDecl(Var, $1, NULL); }
    | fun-decls %prec "declaration"   { $$ = (void*) createDecl(Fun, $1, NULL); }
    | ty-decl                         { $$ = (void*) createDecl(Ty , $1, NULL); }

id-type: INTT                         { $$ = (void*) createType(Int, NULL); }
       | FLTT                         { $$ = (void*) createType(Float, NULL); }
       | STRT                         { $$ = (void*) createType(String, NULL); }
       | ANST                         { $$ = (void*) createType(Answer, NULL); }
       | CNTT                         { $$ = (void*) createType(Cont, NULL); }
       | STRCT                        { $$ = (void*) createType(StrConsumer, NULL); }
       | ID                           { $$ = (void*) createType(Name, $1); }

arg-decl: ID COLL id-type arg-decl2         { $$ = (void*) createParamTy($1, $3, $4); }
        | /* empty */                       { $$ = NULL; }
arg-decl2: arg-decl2 COMM ID COLL id-type   { 
                                              if ($1) {
                                                ParamTyT *Ptr = (ParamTyT*) $1;
                                                while (Ptr->Next) Ptr = Ptr->Next;
                                                Ptr->Next = createParamTy($3, $5, NULL);
                                                $$ = $1;
                                              } else {
                                                $$ = (void*) createParamTy($3, $5, NULL);
                                              }
                                            }
         | /* empty */                      { $$ = NULL; }


/* -= variable declaration =- */

var-decl: VAR ID var-decl2        { $$ = (void*) createVarDecl($2, $3); }
var-decl2: COLL id-type ASGN expr { $$ = $4; }
         | ASGN expr              { $$ = $2; } 

/* -= types declaration =- */

ty-decl: TYPE ID EQ ty-decl2      { $$ = (void*) createTyDecl($2, $4); }
ty-decl2: id-type                 { $$ = $1; }
        | LBRK arg-decl RBRK      { $$ = (void*) createType(ParamTy, $2); }
        | ARRY OF id-type         {
                                    ArrayTyT *Arr = (void*) createArrayTy($3); 
                                    $$ = (void*) createType(ArrayTy, Arr);
                                  }
        | fun-ty-decl             { $$ = (void*) createType(FunTy, $1); }

fun-ty-decl: ty-decl2 INTO ty-decl2         {
                                              TypeT *One = (TypeT*) $1;     
                                              TypeT *Two = (TypeT*) $3;
                                              SeqTyT *From = createSeqTy(One, NULL);
                                              SeqTyT *To   = createSeqTy(Two, NULL);
                                              $$ = (void*) createFunTy(From, To);
                                            }
           | LPAR ty-seq RPAR INTO ty-decl2 {
                                              TypeT  *Ty = (TypeT*) $5;
                                              SeqTyT *To = createSeqTy(Ty, NULL);
                                              $$ = (void*) createFunTy($2, To);
                                            }

ty-seq: ty-decl2 ty-seq2        { $$ = createSeqTy($1, $2); }
      | /* empty */             { $$ = NULL; }
ty-seq2: ty-seq2 COMM ty-decl2  { 
                                  if ($1) {
                                    SeqTyT *Ptr = (SeqTyT*) $1;
                                    while (Ptr->Next) Ptr = Ptr->Next;
                                    Ptr->Next = createSeqTy($3, NULL);
                                    $$ = $1;
                                  } else {
                                    $$ = (void*) createSeqTy($3, NULL);
                                  }
                                }
       | /* empty */            { $$ = NULL; }

/* -= function declaration =- */

fun-decls: fun-decls fun-decl { 
                                FunDeclT *Ptr = (FunDeclT*) $1;
                                while (Ptr->Next) Ptr = Ptr->Next;
                                Ptr->Next = $2;
                                $$ = $1;
                              }
         | fun-decl           { $$ = $1; }
       
fun-decl: FUN ID LPAR arg-decl RPAR fun-decl2 { $$ = (void*) createFunDecl($2, $6, $4, NULL); }
fun-decl2: COLL id-type EQ expr               { $$ = $4; } 
         | EQ expr                            { $$ = $2; } 

/* --------------- if-then-else -------------- */

if-then: IF expr THEN expr ELSE expr          { $$ = (void*) createIfStmt($2, $4, $6); }
       | IF expr THEN expr                    { $$ = (void*) createIfStmt($2, $4, NULL); }

/* ------------- function call --------------- */

fun-call: ID LPAR arg-expr RPAR { $$ = (void*) createFunCall($1, $3); }

arg-expr: expr arg-expr2        { $$ = (void*) createArg($1, $2); }
        | /* empty */           { $$ = NULL; }
arg-expr2: arg-expr2 COMM expr  { 
                                  if ($1) {
                                    ArgT *A = (ArgT*) $1;
                                    while (A->Next) A = A->Next;
                                    A->Next = createArg($3, NULL);
                                    $$ = $1;
                                  } else {
                                    $$ = (void*) createArg($3, NULL);
                                  }
                                }
         | /* empty */          { $$ = NULL; }

/* ------------- parenthesis expr ------------ */

par-expr : LPAR expr RPAR       { $$ = $2; }

/* --------------- creation ------------------ */

create : rec-create             { $$ = (void*) createCreate(Record, $1); }
       | arr-create             { $$ = (void*) createCreate(Array , $1); }

/* -= record creation =- */

rec-create: ID LBRK rec-field RBRK      { $$ = (void*) createRecord($1, $3); }

rec-field: ID EQ expr rec-field2        { $$ = (void*) createField($1, $3, $4); }
         | /* empty */                  { $$ = NULL; }
rec-field2: rec-field2 COMM ID EQ expr  {
                                          if ($1) {
                                            FieldT *Ptr = (FieldT*) $1;
                                            while (Ptr->Next) Ptr = Ptr->Next;
                                            Ptr->Next = createField($3, $5, NULL);
                                            $$ = $1;
                                          } else {
                                            $$ = (void*) createField($3, $5, NULL);
                                          }
                                        }
          | /* empty */                 { $$ = NULL; }

/* -= array creation=- */

arr-create: ID LSQB expr RSQB OF expr  { $$ = (void*) createArray($1, $3, $6); }

/* ------------------------------------------- */

%%

extern FILE *yyin;

void yyerror(const char *s, ...) {
  va_list Args;
  va_start(Args, s);

  printf("@[%d,%d]: ", yylloc.first_line, yylloc.first_column);
  vprintf(s, Args);
  printf("\n");

  va_end(Args);
}

int main(int argc, char **argv) {
  if (argc > 1) yyin = fopen(argv[1], "r");
  int parsing = yyparse();
  return parsing;
}
