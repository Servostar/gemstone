%{
    #include <sys/log.h>
    extern int yylineno;

    int yyerror(char*);

    extern int yylex();
%}

%union {
    char *string;
}

%token KeyInt
%token KeyFloat
%token KeyAs
%token <string> ValInt
%token <string> Ident
%token <string> ValFloat 
%token <string> ValStr
%token <string> ValMultistr
%token KeyShort
%token KeyLong
%token KeyHalf
%token KeyDouble
%token KeySigned
%token KeyUnsigned
%token KeyRef
%token KeyType
%token KeyLocal
%token KeyGlobal
%token KeyStatic
%token KeyIf
%token KeyElse
%token KeyWhile
%token KeyIn
%token KeyOut
%token KeyFun
%token OpEquals
%token OpAnd
%token OpOr
%token OpNot
%token OpXor
%token OpBitand
%token OpBitor
%token OpBitnot
%token OpBitxor
%token KeyImport
%token KeySilent
%token KeyBox
%token FunTypeof
%token FunSizeof
%token FunFilename
%token FunFunname
%token FunLineno
%token FunExtsupport

%%
program: funcall;

expr: ValFloat
    | ValInt
    | ValMultistr
    | ValStr
    | Ident;

exprlist: expr ',' exprlist
        | expr
        | ;

paramlist: paramlist '(' exprlist ')'
    | ;

funcall: Ident paramlist { DEBUG("Function call"); };

assign: Ident '=' expr { DEBUG("Assignment"); };

identlist: Ident ',' identlist
        | Ident
        | ;

decl: type ':' identlist { DEBUG("Declaration"); };

definition: decl '=' expr { DEBUG("Definition"); };

sign: KeySigned
    | KeyUnsigned
    | ;

scale: scale KeyShort
    | scale KeyHalf
    | scale KeyLong
    | scale KeyDouble
    | ;

type: sign scale Ident
    | sign scale KeyInt
    | sign scale KeyFloat;



%%

int yyerror(char *s) {
    ERROR("%s", s);
    return 0;
}