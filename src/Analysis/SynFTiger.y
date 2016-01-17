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

%precedence FEQ

%precedence DECL
%precedence FUN

%left OR
%left AND
%left EQ DIF LT LE GT GE
%left PLS MIN
%left DIV MUL
%right UMIN

%type <Node> expr 

/* expr */
%type <Node> lit lVal 

/* lval */
%type <Node> record array

%start program

%%

program: expr
       | /* empty */

expr   : lit                      { $$ = NULL; }
       | lVal                     { $$ = NULL; }
       | arithm                   { $$ = NULL; }
       | unary                    { $$ = NULL; }
       | compare                  { $$ = NULL; }
       | logic                    { $$ = NULL; }
       | let                      { $$ = NULL; }
       | if-then                  { $$ = NULL; }
       | funCall                  { $$ = NULL; }
       | parExp                   { $$ = NULL; }
       | create                   { $$ = NULL; }
       | NIL                      { $$ = NULL; }
       | error { printf("\t%s: Invalid expression.\n", SynError); YYABORT; }

/* ----------- primitive types --------------- */

lit    : INT                      { $$ = (void*) createLit(Int, &($1)); }
       | FLT                      { $$ = (void*) createLit(Float, &($1)); }
       | STR                      { $$ = (void*) createLit(String, $1); }
       | error PT INT { printf("\t%s: Invalid real expression.\n", SynError); YYABORT; }

/* ----------------- l-value ----------------- */

lVal   : record                   { $$ = $1; }
       | array                    { $$ = $1; }
       | ID %prec LVAL            { $$ = (void*) createLval(Id, $1, NULL, NULL); }

record : lVal PT ID               { $$ = (void*) createLval(RecordAccess, $3, $1, NULL); }

array  : lVal LSQB expr RSQB      { $$ = (void*) createLval(ArrayAccess, NULL, $1, $3); }

/* ---------------- arithmetic --------------- */

arithm : expr PLS expr
       | expr MIN expr
       | expr MUL expr
       | expr DIV expr

unary  : MIN expr %prec UMIN

/* --------------- comparison ---------------- */

compare: expr EQ expr 
       | expr DIF expr 
       | expr LT expr 
       | expr LE expr 
       | expr GT expr 
       | expr GE expr 

/* ------------------ logic ------------------ */

logic  : expr AND expr 
       | expr OR expr 

/* ------------------ let -------------------- */

let    : LET decls IN expr END

/* --------------- declarations -------------- */

decls  : decls decl 
       | /* empty */

decl   : varDec
       | funDecs %prec DECL
       | tyDec
       | error { printf("\t%s: Invalid declaration.\n", SynError); YYABORT; }

idType : INTT
       | FLTT
       | STRT
       | ANST
       | CNTT
       | STRCT
       | ID

argDec : ID COLL idType argDec2
       | /* empty */
argDec2: argDec2 commErr ID COLL idType
       | /* empty */

commErr: COMM
       | error { printf("\t%s: Invalid argument declaration.\n", SynError); YYABORT; }

/* -= variable declaration =- */

varDec : VAR ID varDec2
varDec2: COLL idType opErr expr 
       | opErr expr 

/* -= errors =- */
opErr  : ASGN
       | error { printf("\t%s: Expected ':='.\n", SynError); YYABORT; }

/* -= types declaration =- */

tyDec  : TYPE ID EQ tyDec2
tyDec2 : idType
       | LBRK argDec RBRK
       | ARRY OF idType
       | fTyDec

fTyDec : tyDec2 INTO tyDec2
       | LPAR tySeq RPAR INTO tyDec2

tySeq  : tyDec2 tySeq2
       | /* empty */
tySeq2 : tySeq2 COMM tyDec2
       | /* empty */

/* -= function declaration =- */

funDecs: funDecs funDec
       | funDec
       
funDec: FUN ID LPAR argDec RPAR funDec2
funDec2: COLL idType EQ expr %prec FEQ
       | EQ expr %prec FEQ

/* --------------- if-then-else -------------- */

if-then: IF expr THEN expr ELSE expr
       | IF expr THEN expr

/* ------------- function call --------------- */

funCall: ID LPAR argExp RPAR

argExp : expr argExp2
       | /* empty */
argExp2: argExp2 COMM expr 
       | /* empty */

/* ------------- parenthesis expr ------------ */

parExp : LPAR expr parExp2
parExp2: RPAR
       | error { printf("\t%s: Parenthesis missing.\n", SynError); YYABORT; }
/* --------------- creation ------------------ */

create : rCreat
       | aCreat

/* -= record creation =- */

rCreat : ID LBRK rField RBRK

rField : ID EQ expr rField2
       | /* empty */
rField2: rField2 COMM ID EQ expr 
       | /* empty */

/* -= array creation=- */

aCreat : ID LSQB limErr OF expr 
limErr : expr RSQB 
       | error { printf("\t%s: Invalid array initialization.\n", SynError); YYABORT; }

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
