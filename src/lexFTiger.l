%{

#include <stdio.h>

#define INT 1
#define IDENT 2
#define IF 3

%}

CHAR [a-zA-Z]
DIGIT [0-9]

%%

if                        {return IF;}
{DIGIT}+                  {return INT;}
{CHAR}+({DIGIT}|{CHAR})*  {return IDENT;}
[ \n\t\r]                 ;
.                         {printf("Ooops\n");}

%%

int main() {
  int n;
  char *tk[] = { "int", "ident", "if"};
  while ((n = yylex())) {
    printf("Token %s: %s\n", tk[n-1], yytext); 
  }

  return 0;
}
