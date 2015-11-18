%{

#include "Tokens.h"
#include "UtilLex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

%}

%option stack

%x STRING ESCAPE CONTROL
%x COMMENT

BLNK  [\n\r\t ]

ASCIIC [1-9][0-9]{1,2}
MLSTR {BLNK}+\\

NESC  [^nt\^\"{ASCIIC}{MLSTR}]
NCTR  [^\@G-M\?]

%%

%{
int   StringBufMax;
char *StringBuf;
char *StringBufPtr;
%}

([1-9]+[0-9]*|"0")        { return INT; }
[0-9]+"."[0-9]+           { return FLT; }

\"                        { 
                            yy_push_state(STRING);     
                            StringBufMax = STDLEN;
                            StringBuf    = (char*) malloc(STDLEN);
                            StringBufPtr = StringBuf;
                          }

<STRING>{
\"                        { 
                            yy_pop_state(); 
                            *StringBufPtr = '\0';
                            return STR; 
                          }
\\                        { yy_push_state(ESCAPE); }
[^\"\\]+                  { 
                            copyToBuffer(&StringBuf, &StringBufPtr, 
                              &StringBufMax, yytext);
                          }
}

<ESCAPE>{
{BLNK}+\\                 { yy_pop_state(); }
n                         { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\n');
                            yy_pop_state();
                          }
t                         { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\t');
                            yy_pop_state();
                          }
[1-9]+[0-9]{1,2}          { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, atoi(yytext));
                            yy_pop_state();
                          } 
\^                        { yy_push_state(CONTROL); }
{NESC}                    {   
                            printf("LexicalError: Escaped character not recognized: \\%s.\n", yytext); 
                            yy_pop_state();
                          }
}

<CONTROL>{
\@                        { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\0');
                            yy_pop_state();
                            yy_pop_state();
                          }
G                         { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\a');
                            yy_pop_state();
                            yy_pop_state();
                          }
H                         { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\b');
                            yy_pop_state();
                            yy_pop_state();
                          }
I                         { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\t');
                            yy_pop_state();
                            yy_pop_state();
                          }
J                         { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\n');
                            yy_pop_state();
                            yy_pop_state();
                          }
K                         { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\v');
                            yy_pop_state();
                            yy_pop_state();
                          }
L                         { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\f');
                            yy_pop_state();
                            yy_pop_state();
                          }
M                         { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\r');
                            yy_pop_state();
                            yy_pop_state();
                          }
\?                        { 
                            copyToBufferChar(&StringBuf, &StringBufPtr, 
                              &StringBufMax, '\127');
                            yy_pop_state();
                            yy_pop_state();
                          }
{NCTR}                    { 
                            printf("LexicalError: Control character not recognized: \\^%s.\n", yytext); 
                            yy_pop_state();
                            yy_pop_state();
                          }
}

"int"                     { return INTT; }
"float"                   { return FLTT; }
"string"                  { return STRT; }

"for"                     { return FOR;  }
"to"                      { return TO;   }
"break"                   { return BRK;  }
"let"                     { return LET;  }
"in"                      { return IN;   }
"end"                     { return END;  }
"function"                { return FUN;  }
"var"                     { return VAR;  }
"if"                      { return IF;   }
"then"                    { return THEN; }
"else"                    { return ELSE; }
"do"                      { return DO;   }
"while"                   { return WHL;  }

"/*"                      { yy_push_state(COMMENT); }
<COMMENT>"*/"             { yy_pop_state(); }
<COMMENT>[^("*/")]

"+"                       { return PLS;   }
"-"                       { return MIN;   }
"/"                       { return DIV;   }
"*"                       { return MUL;   }
":="                      { return ASGN;  }
"="                       { return EQ;    }
"<>"                      { return DIF;   }
"<"                       { return LT;    }
"<="                      { return LE;    }
">"                       { return GT;    }
">="                      { return GE;    }
"&"                       { return AND;   }
"|"                       { return OR;    }

[\(\);:,]                 { /* let it go */ }

[a-zA-Z]+[0-9a-zA-Z_]*    { return ID; }

{BLNK}                    { /* eat some spaces */ }

.                         { printf("LexicalError: Token not recognized: %s.\n", yytext); }

%%

int main(int argc, char **argv) {
  int n;
  if (argc > 1) {
    printf("File: %s\n", argv[1]);
    yyin = fopen(argv[1], "r");
  }
  while ((n = yylex())) {
    printf("Token No: %d\n", n);
  }
  return 0;
}