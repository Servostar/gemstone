%{
    extern int yylineno;

    int yyerror(char*);

    extern int yylex();
%}

%%
program: ;
%%

int yyerror(char *s) {
    return 0;
}