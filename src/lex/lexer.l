%option noyywrap
%{
    #include <yacc/parser.tab.h>

    int yyLineNumber = 1;
    int yylex();
%}

%%
.;
%%