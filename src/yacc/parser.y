%{
    #include <sys/log.h>
    extern int yylineno;

    int yyerror(char*);

    extern int yylex();
%}

%%
program: ;
%%

int yyerror(char *s) {
    ERROR("%s", s);
    return 0;
}