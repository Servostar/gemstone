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

/* Operator associativity */
%right '='
%left '+' '-' '*' '/'
%left OpEquals OpNot '<' '>'
%left OpAnd OpOr OpXor
%left OpBitand OpBitor OpBitxor OpBitnot

%%
program: assign
    | definition;

expr: ValFloat
    | ValInt
    | ValMultistr
    | ValStr
    | Ident
    | operation;

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

operation: oparith
    | oplogic
    | opbool
    | opbit;

oparith: expr '+' expr
    | expr '-' expr
    | expr '*' expr
    | expr '/' expr
    | '-' expr %prec '*';

oplogic: expr OpEquals expr
    | expr '<' expr
    | expr '>' expr;

opbool: expr OpAnd expr
    | expr OpOr expr
    | expr OpXor expr
    | OpNot expr %prec OpAnd;

opbit: expr OpBitand expr
    | expr OpBitor expr
    | expr OpBitxor expr
    | OpBitnot expr %prec OpBitand;

%%

int yyerror(char *s) {
    ERROR("%s", s);
    return 0;
}