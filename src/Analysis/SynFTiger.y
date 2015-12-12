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
       | '(' parExp ')'
       | arithm
       | logic
       | if-then
       | WHL expr DO expr
       | FOR decl TO expr DO expr
       | BRK
       | LET decls IN parExp END
       | ID '(' seqExp ')' 

parExp : seqExp
       | expr
       | /* empty */

/* ------------- sequential expr ------------- */

seqExp : expr ';' expr seqExp2 
seqExp2: ';' expr seqExp2
       | /* empty */

/* ---------------- arithmetic --------------- */

arithm : expr PLS expr
       | expr MIN expr
       | expr MUL expr
       | expr DIV expr

/* ------------------ logic ------------------ */

logic  : expr EQ expr 
       | expr DIF expr 
       | expr LT expr 
       | expr LE expr 
       | expr GT expr 
       | expr GE expr 
       | expr AND expr 
       | expr OR expr 

/* --------------- if-then-else -------------- */

if-then: IF expr THEN expr ELSE expr
       | IF expr THEN expr

/* --------------- declarations -------------- */

decl   : varDec
       | funDec

decls  : decl decls
       | /* empty */

funDec : FUN ID '(' params ')' funDec2
funDec2: ':' idType EQ expr
       | EQ expr

params : ID ':' idType params2
       | /* empty */
params2: ',' ID ':' idType params2
       | /* empty */

varDec : VAR ID varDec2
varDec2: ':' idType ASGN expr
       | ASGN expr

idType : INTT
       | FLTT
       | STRT

/* ------------------ types ------------------ */

lit    : id
       | num
       | STR

id     : ID
       | '-' ID %prec UMIN

numType: INT
       | FLT
num    : numType
       | '-' numType %prec UMIN

/* ------------------------------------------- */

%%

void yyerror(void) {
  printf("Syntax error.");
}

int main() {

}
