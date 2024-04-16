%{
    extern int yylineno;

    int yyerror(char*);

    extern int yylex();
%}

%union {
    char *string;
    int num;
    float real;
}

%token KeyInt
%token KeyFloat
%token KeyAs
%token <num> ValInt
%token <string> Ident
%token <real> ValFloat 
%token <string> ValStr
%token <string> ValMultistr
%token KeyShort
%token KeyLong
%token KeyHalf
%token KeyDouble
%token KeySigned
%token Keyunsigned
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
program: ;
%%

int yyerror(char *s) {
    return 0;
}