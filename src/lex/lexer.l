%option noyywrap
%{
    #include <yacc/parser.tab.h>

    int yyLineNumber = 1;
    int yylex();
%}

/* disable the following functions */
/* to avoid failing code check */
%option nounput
%option noinput

%%
.;
%%
