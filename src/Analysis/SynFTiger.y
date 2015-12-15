%{
  #include <stdio.h>
  int yylex(void);
  void yyerror(char const *);
%}

%token INT FLT STR

%token FOR TO BRK LET IN END FUN VAR IF THEN ELSE DO NIL WHL
%token PLS MIN DIV MUL ASGN EQ DIF LT LE GT GE AND OR

%token ID
%token INTT FLTT STRT

%token UMIN

%nonassoc THEN
%nonassoc ELSE

%nonassoc DO

%left OR
%left AND
%left EQ DIF LT LE GT GE
%left PLS MIN
%left DIV MUL
%right UMIN

%start program

%%

program: expr

expr   : lit
       | arithm
       | compare
       | logic
       | let
       | if-then
       | funCall
       | parExp

/* ------------------ let -------------------- */

let    : LET decls IN expr END

/* ------------- function call --------------- */

funCall: ID '(' argExp ')'

argExp : expr argExp2
       | /* empty */
argExp2: ',' expr argExp2
       | /* empty */

/* ------------- parenthesis expr ------------ */

parExp : '(' parExp2 ')'
parExp2: expr
       | /* empty */

/* ---------------- arithmetic --------------- */

arithm : expr PLS expr
       | expr MIN expr
       | expr MUL expr
       | expr DIV expr

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

/* --------------- if-then-else -------------- */

if-then: IF expr THEN expr ELSE expr
       | IF expr THEN expr

/* --------------- declarations -------------- */

decl   : varDec
       | funDec

decls  : decl decls
       | /* empty */

idType : INTT
       | FLTT
       | STRT

/* -= function declaration =- */

funDec : FUN ID '(' argDec ')' funDec2
funDec2: ':' idType EQ expr
       | EQ expr

argDec : ID ':' idType argDec2
       | /* empty */
argDec2: ',' ID ':' idType argDec2
       | /* empty */

/* -= variable declaration =- */

varDec : VAR ID varDec2
varDec2: ':' idType ASGN expr
       | ASGN expr

/* ------------------ types ------------------ */

lit    : id
       | num
       | STR

id     : ID
       | '-' ID %prec UMIN

num    : numType
       | '-' numType %prec UMIN
numType: INT
       | FLT

/* ------------------------------------------- */

%%

void yyerror(const char *s) {
  printf("Syntax error: %s.", s);
}

int main() {

}
