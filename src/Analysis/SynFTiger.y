%{
#include <stdio.h>
#include <stdarg.h>

#include "ftc/Analysis/ParserTree.h"

int yylex(void);

static const char LexError[] = "LexicalError";
static const char SynError[] = "SyntaticalError";

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
  ASTNode *Node;
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

%type <Node> expr lit l-val arithm unary compare logic let

%type <Node> record array decls decl var-decl fun-decl ty-decl id-type arg-decl
%type <Node> fun-ty-decl ty-seq

%type <Node> arg-decl2 var-decl2 ty-decl2 ty-seq2

%start program

%%

program: expr
       | /* empty */

expr: lit                   { $$ = NULL; }
    | l-val                 { $$ = NULL; }
    | arithm                { $$ = NULL; }
    | unary                 { $$ = NULL; }
    | compare               { $$ = NULL; }
    | logic                 { $$ = NULL; }
    | let                   { $$ = NULL; }
    | if-then               { $$ = NULL; }
    | fun-call              { $$ = NULL; }
    | par-expr              { $$ = NULL; }
    | create                { $$ = NULL; }
    | NIL                   { $$ = NULL; }

/* ----------- primitive types --------------- */

lit: INT                    { 
                              int *Int = (int*) malloc(sizeof(int));
                              *Int = $1;
                              $$ = createASTNode(IntLit, Int, NULL);
                            }
   | FLT                    { 
                              float *Float = (float*) malloc(sizeof(float));
                              *Float = $1;
                              $$ = createASTNode(FloatLit, Float, NULL);
                            }
   | STR                    { $$ = createASTNode(StringLit, $1, NULL); }

/* ----------------- l-value ----------------- */

l-val: record                 { $$ = $1; }
     | array                  { $$ = $1; } 
     | ID %prec LVAL          { $$ = createASTNode(IdLval, $1, NULL); }

record: l-val PT ID           { $$ = createASTNode(RecAccessLval, $3, $1, NULL); }

array: l-val LSQB expr RSQB   { $$ = createASTNode(ArrAccessLval, NULL, $1, $3, NULL); }

/* ---------------- arithmetic --------------- */

arithm: expr PLS expr         { $$ = createASTNode(SumOp , NULL, $1, $3, NULL); }
      | expr MIN expr         { $$ = createASTNode(SubOp , NULL, $1, $3, NULL); }
      | expr MUL expr         { $$ = createASTNode(MultOp, NULL, $1, $3, NULL); }
      | expr DIV expr         { $$ = createASTNode(DivOp , NULL, $1, $3, NULL); }

unary: MIN expr %prec UMIN    { $$ = createASTNode(NegOp, NULL, $2, NULL); }

/* --------------- comparison ---------------- */

compare: expr EQ expr         { $$ = createASTNode(EqOp  , NULL, $1, $3, NULL); }
       | expr DIF expr        { $$ = createASTNode(DiffOp, NULL, $1, $3, NULL); }
       | expr LT expr         { $$ = createASTNode(LtOp  , NULL, $1, $3, NULL); }
       | expr LE expr         { $$ = createASTNode(LeOp  , NULL, $1, $3, NULL); }
       | expr GT expr         { $$ = createASTNode(GtOp  , NULL, $1, $3, NULL); }
       | expr GE expr         { $$ = createASTNode(GeOp  , NULL, $1, $3, NULL); }

/* ------------------ logic ------------------ */

logic: expr AND expr          { $$ = createASTNode(AndOp, NULL, $1, $3, NULL); }
     | expr OR expr           { $$ = createASTNode(OrOp , NULL, $1, $3, NULL); }

/* ------------------ let -------------------- */

let: LET decls IN expr END    { $$ = createASTNode(LetExpr, NULL, $2, $4, NULL); } 

/* --------------- declarations -------------- */

decls: decls decl             { }
     | /* empty */            { $$ = NULL; }

decl: var-decl                      { $$ = $1; }
    | fun-decls %prec "declaration" { $$ = $1; }
    | ty-decl                       { $$ = $1; }

id-type: INTT                 { $$ = createASTNode(IntTy   , NULL, NULL); }
       | FLTT                 { $$ = createASTNode(FloatTy , NULL, NULL); }
       | STRT                 { $$ = createASTNode(StringTy, NULL, NULL); }
       | ANST                 { $$ = createASTNode(AnswerTy, NULL, NULL); }
       | CNTT                 { $$ = createASTNode(ContTy  , NULL, NULL); }
       | STRCT                { $$ = createASTNode(StrConsumerTy, NULL, NULL); }
       | ID                   { $$ = createASTNode(IdTy    , $1  , NULL); }

arg-decl: ID COLL id-type arg-decl2       
        | /* empty */
arg-decl2: arg-decl2 COMM ID COLL id-type
         | /* empty */


/* -= variable declaration =- */

var-decl: VAR ID var-decl2        { $$ = createASTNode(VarDecl, $2, $3, NULL); }
var-decl2: COLL id-type ASGN expr { $$ = $4; }
         | ASGN expr              { $$ = $2; }

/* -= types declaration =- */

ty-decl: TYPE ID EQ ty-decl2      { $$ = createASTNode(TyDecl, $2, $4, NULL); }
ty-decl2: id-type                 { $$ = $1; }
        | LBRK arg-decl RBRK      { $$ = $2; }
        | ARRY OF id-type         { $$ = createASTNode(ArrayTy, NULL, $3, NULL); }
        | fun-ty-decl             { $$ = $1; }

fun-ty-decl: ty-decl2 INTO ty-decl2         { $$ = createASTNode(FunTy, NULL, $1, $3, NULL); }
           | LPAR ty-seq RPAR INTO ty-decl2 { $$ = createASTNode(FunTy, NULL, $2, $5, NULL); }

ty-seq: ty-decl2 ty-seq2
      | /* empty */
ty-seq2: ty-seq2 COMM ty-decl2
       | /* empty */

/* -= function declaration =- */

fun-decls: fun-decls fun-decl
         | fun-decl
       
fun-decl: FUN ID LPAR arg-decl RPAR fun-decl2
fun-decl2: COLL id-type EQ expr 
         | EQ expr 

/* --------------- if-then-else -------------- */

if-then: IF expr THEN expr ELSE expr
       | IF expr THEN expr

/* ------------- function call --------------- */

fun-call: ID LPAR arg-expr RPAR

arg-expr: expr arg-expr2
        | /* empty */
arg-expr2: arg-expr2 COMM expr 
         | /* empty */

/* ------------- parenthesis expr ------------ */

par-expr : LPAR expr RPAR

/* --------------- creation ------------------ */

create : rec-create
       | arr-create

/* -= record creation =- */

rec-create: ID LBRK rec-field RBRK

rec-field: ID EQ expr rec-field2
         | /* empty */
rec-field2: rec-field2 COMM ID EQ expr 
          | /* empty */

/* -= array creation=- */

arr-create: ID LSQB expr RSQB OF expr 

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
