%{
#include <stdio.h>
#include <stdarg.h>

#include "ftc/Analysis/ParserTree.h"
#include "ftc/Analysis/DrawTree.h"

int yylex(void);

static const char LexError[] = "LexicalError";
static const char SynError[] = "SyntaticalError";

ASTNode *Root;
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
  void *Vector;
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

%type <Node> program expr lit l-val record array arithm unary compare logic let 
%type <Node> decl id-type arg-decl var-decl ty-decl ty-decl2 fun-ty-decl 
%type <Node> ty-seq fun-decl if-then fun-call arg-expr rec-create 
%type <Node> rec-field arr-create par-expr create

%type <Vector> decls arg-decl2 ty-seq2 fun-decls arg-expr2 rec-field2 var-decl2 fun-decl2

%start program

%%

program: expr               { Root = $1; }
       | /* empty */        { $$ = NULL; }

expr: lit                   { $$ = $1; }
    | l-val                 { $$ = $1; }
    | arithm                { $$ = $1; }
    | unary                 { $$ = $1; }
    | compare               { $$ = $1; }
    | logic                 { $$ = $1; }
    | let                   { $$ = $1; }
    | if-then               { $$ = $1; }
    | fun-call              { $$ = $1; }
    | par-expr              { $$ = $1; }
    | create                { $$ = $1; }
    | NIL                   { $$ = createASTNode(NilExpr, NULL, 0); }

/* ----------- primitive types --------------- */

lit: INT                    { 
                              int *Int = (int*) malloc(sizeof(int));
                              *Int = $1;
                              $$ = createASTNode(IntLit, Int, 0);
                            }
   | FLT                    { 
                              float *Float = (float*) malloc(sizeof(float));
                              *Float = $1;
                              $$ = createASTNode(FloatLit, Float, 0);
                            }
   | STR                    { $$ = createASTNode(StringLit, $1, 0); }

/* ----------------- l-value ----------------- */

l-val: record                 { $$ = $1; }
     | array                  { $$ = $1; } 
     | ID %prec LVAL          { $$ = createASTNode(IdLval, $1, 0); }

record: l-val PT ID           { $$ = createASTNode(RecAccessLval, $3, 1, $1); }

array: l-val LSQB expr RSQB   { $$ = createASTNode(ArrAccessLval, NULL, 2, $1, $3); }

/* ---------------- arithmetic --------------- */

arithm: expr PLS expr         { $$ = createASTNode(SumOp , NULL, 2, $1, $3); }
      | expr MIN expr         { $$ = createASTNode(SubOp , NULL, 2, $1, $3); }
      | expr MUL expr         { $$ = createASTNode(MultOp, NULL, 2, $1, $3); }
      | expr DIV expr         { $$ = createASTNode(DivOp , NULL, 2, $1, $3); }

unary: MIN expr %prec UMIN    { $$ = createASTNode(NegOp, NULL, 1, $2); }

/* --------------- comparison ---------------- */

compare: expr EQ expr         { $$ = createASTNode(EqOp  , NULL, 2, $1, $3); }
       | expr DIF expr        { $$ = createASTNode(DiffOp, NULL, 2, $1, $3); }
       | expr LT expr         { $$ = createASTNode(LtOp  , NULL, 2, $1, $3); }
       | expr LE expr         { $$ = createASTNode(LeOp  , NULL, 2, $1, $3); }
       | expr GT expr         { $$ = createASTNode(GtOp  , NULL, 2, $1, $3); }
       | expr GE expr         { $$ = createASTNode(GeOp  , NULL, 2, $1, $3); }

/* ------------------ logic ------------------ */

logic: expr AND expr          { $$ = createASTNode(AndOp, NULL, 2, $1, $3); }
     | expr OR expr           { $$ = createASTNode(OrOp , NULL, 2, $1, $3); }

/* ------------------ let -------------------- */

let: LET decls IN expr END    { 
                                // Declaration Node
                                ASTNode *Node = createASTNode(DeclList, NULL, 0);
                                if ($2) moveAllToASTNode(Node, $2);
                                // Let Node
                                $$ = createASTNode(LetExpr, NULL, 2, Node, $4); 
                              } 

/* --------------- declarations -------------- */

decls: decls decl             { 
                                if ($1) $$ = $1;
                                else $$ = createASTNodeVector();
                                appendASTNode($$, $2); 
                              }
     | /* empty */            { $$ = NULL; }

decl: var-decl                      { $$ = $1; }
    | fun-decls %prec "declaration" { 
                                      $$ = createASTNode(FunDeclList, NULL, 0); 
                                      moveAllToASTNode($$, $1);
                                    }
    | ty-decl                       { $$ = $1; }

id-type: INTT                 { $$ = createASTNode(IntTy   , NULL, 0); }
       | FLTT                 { $$ = createASTNode(FloatTy , NULL, 0); }
       | STRT                 { $$ = createASTNode(StringTy, NULL, 0); }
       | ANST                 { $$ = createASTNode(AnswerTy, NULL, 0); }
       | CNTT                 { $$ = createASTNode(ContTy  , NULL, 0); }
       | STRCT                { $$ = createASTNode(StrConsumerTy, NULL, 0); }
       | ID                   { $$ = createASTNode(IdTy    , $1  , 0); }

arg-decl: ID COLL id-type arg-decl2       { 
                                            ASTNode *Node = createASTNode(ArgDecl, $1, 1, $3); 
                                            $$ = createASTNode(ArgDeclList, NULL, 1, Node);
                                            if ($4) moveAllToASTNode($$, $4);
                                          }
        | /* empty */                     { $$ = NULL; }
arg-decl2: arg-decl2 COMM ID COLL id-type {
                                            if ($1) $$ = $1; 
                                            else $$ = createASTNodeVector();
                                            ASTNode *Node = createASTNode(ArgDecl, $3, 1, $5); 
                                            appendASTNode($$, Node);
                                          }
         | /* empty */                    { $$ = NULL; }


/* -= variable declaration =- */

var-decl: VAR ID var-decl2        {
                                    $$ = createASTNode(VarDecl, $2, 0); 
                                    moveAllToASTNode($$, $3);
                                  }
var-decl2: COLL id-type ASGN expr { 
                                    $$ = createASTNodeVector(); 
                                    appendASTNode($$, $2);
                                    appendASTNode($$, $4);
                                  }
         | ASGN expr              { 
                                    $$ = createASTNodeVector(); 
                                    appendASTNode($$, $2);
                                  } 

/* -= types declaration =- */

ty-decl: TYPE ID EQ ty-decl2      { $$ = createASTNode(TyDecl, $2, 1, $4); }
ty-decl2: id-type                 { $$ = $1; }
        | LBRK arg-decl RBRK      { $$ = $2; }
        | ARRY OF id-type         { $$ = createASTNode(ArrayTy, NULL, 1, $3); }
        | fun-ty-decl             { $$ = $1; }

fun-ty-decl: ty-decl2 INTO ty-decl2         { $$ = createASTNode(FunTy, NULL, 2, $1, $3); }
           | LPAR ty-seq RPAR INTO ty-decl2 { $$ = createASTNode(FunTy, NULL, 2, $2, $5); }

ty-seq: ty-decl2 ty-seq2        {
                                  ASTNode *Node = createASTNode(TySeq, NULL, 1, $1); 
                                  $$ = createASTNode(TySeqList, NULL, 1, Node);
                                  if ($2) moveAllToASTNode($$, $2);
                                }
      | /* empty */             { $$ = NULL; }
ty-seq2: ty-seq2 COMM ty-decl2  {
                                  if ($1) $$ = $1;
                                  else $$ = createASTNodeVector();
                                  ASTNode *Node = createASTNode(TySeq, NULL, 1, $3);
                                  appendASTNode($$, Node);
                                }
       | /* empty */            { $$ = NULL; }

/* -= function declaration =- */

fun-decls: fun-decls fun-decl                 { appendASTNode($1, $2); $$ = $1; }
         | fun-decl                           {
                                                $$ = createASTNodeVector();
                                                appendASTNode($$, $1);
                                              }
       
fun-decl: FUN ID LPAR arg-decl RPAR fun-decl2 { 
                                                $$ = createASTNode(FunDecl, $2, 1, $4); 
                                                moveAllToASTNode($$, $6);
                                              }
fun-decl2: COLL id-type EQ expr               { 
                                                $$ = createASTNodeVector();
                                                appendASTNode($$, $2);
                                                appendASTNode($$, $4);
                                              }
         | EQ expr                            { 
                                                $$ = createASTNodeVector(); 
                                                appendASTNode($$, $2);
                                              }

/* --------------- if-then-else -------------- */

if-then: IF expr THEN expr ELSE expr    { $$ = createASTNode(IfStmtExpr, NULL, 3, $2, $4, $6); } 
       | IF expr THEN expr              { $$ = createASTNode(IfStmtExpr, NULL, 2, $2, $4); }

/* ------------- function call --------------- */

fun-call: ID LPAR arg-expr RPAR         { $$ = createASTNode(FunCallExpr, $1, 1, $3); } 

arg-expr: expr arg-expr2                {
                                          $$ = createASTNode(ArgExprList, NULL, 1, $1);
                                          if ($2) moveAllToASTNode($$, $2);
                                        }
        | /* empty */                   { $$ = NULL; }
arg-expr2: arg-expr2 COMM expr          { 
                                          if ($1) $$ = $1;
                                          else $$ = createASTNodeVector();
                                          appendASTNode($$, $3); 
                                        }
         | /* empty */                  { $$ = NULL; }

/* ------------- parenthesis expr ------------ */

par-expr : LPAR expr RPAR               { $$ = $2; }

/* --------------- creation ------------------ */

create : rec-create                     { $$ = $1; } 
       | arr-create                     { $$ = $1; }

/* -= record creation =- */

rec-create: ID LBRK rec-field RBRK      { $$ = createASTNode(RecordExpr, $1, 1, $3); }

rec-field: ID EQ expr rec-field2        { 
                                          ASTNode *Node = createASTNode(FieldExpr, $1, 1, $3);
                                          $$ = createASTNode(FieldExprList, NULL, 1, Node);
                                          if ($4) moveAllToASTNode($$, $4);
                                        } 
         | /* empty */                  { $$ = NULL; }
rec-field2: rec-field2 COMM ID EQ expr  { 
                                          if ($1) $$ = $1;
                                          else $$ = createASTNodeVector();
                                          ASTNode *Node = createASTNode(FieldExpr, $3, 1, $5);
                                          appendASTNode($$, Node); 
                                        }
          | /* empty */                 { $$ = NULL; }

/* -= array creation=- */

arr-create: ID LSQB expr RSQB OF expr   { $$ = createASTNode(ArrayExpr, $1, 2, $3, $6); }

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
  drawDotTree("MyDot.dot", Root);
  destroyASTNode(Root);
  return parsing;
}
