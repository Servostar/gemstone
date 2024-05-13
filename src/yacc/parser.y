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
%token KeySelf
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
%nonassoc KeyAs '(' ')'

%%
program: program programbody
       | programbody;

programbody: moduleimport
       | fundef
       | box
       | definition
       | decl
       | typedef;

expr: ValFloat
    | ValInt
    | ValMultistr
    | ValStr
    | Ident
    | operation
    | boxaccess
    | boxselfaccess
    | typecast
    | reinterpretcast;

exprlist: expr ',' exprlist
        | expr;

argumentlist: argumentlist '(' exprlist ')'
    | ;


fundef: KeyFun Ident paramlist '{' statementlist'}' { DEBUG("Function");};

paramlist: paramlist '(' params ')'
         |  paramlist '(' ')'
         | '(' params ')'
         | '(' ')';

params: IOqualifyier paramdecl ',' params
      | IOqualifyier paramdecl;

IOqualifyier: KeyIn
            | KeyOut
            | KeyIn KeyOut
            | KeyOut KeyIn
            | ;

typecast: expr KeyAs type { DEBUG("Type-Cast"); };

reinterpretcast: '(' type ')' expr { DEBUG("Reinterpret-Cast"); };

paramdecl: type ':' Ident { DEBUG("Param-Declaration"); };

box: KeyType KeyBox ':' Ident '{' boxbody '}' { DEBUG("Box"); }
   | KeyType KeyBox ':' Ident '{' '}';

boxbody: boxbody boxcontent
       | boxcontent;

boxcontent: decl { DEBUG("Box decl Content"); }
          | definition { DEBUG("Box def Content"); }
          | fundef { DEBUG("Box fun Content"); };

boxselfaccess: KeySelf '.' Ident
             | KeySelf '.' boxaccess;

boxaccess: Ident '.' Ident
         | Ident '.' boxaccess;

boxcall: boxaccess argumentlist
       | boxselfaccess argumentlist;

funcall: Ident argumentlist { DEBUG("Function call"); };

moduleimport: KeyImport ValStr { DEBUG("Module-Import"); };

statementlist: statement statementlist
    | statement;

statement: assign
        | decl
        | definition
        | while
        | branch
        | funcall
        | boxcall;

branchif: KeyIf expr '{' statementlist '}' { DEBUG("if"); };
branchelse: KeyElse '{' statementlist '}' { DEBUG("if-else"); };
branchelseif: KeyElse KeyIf expr '{' statementlist '}' { DEBUG("else-if"); };

branchelseifs: branchelseifs branchelseif
    | branchelseif;

branch: branchif branchelseifs
    | branchif branchelseifs branchelse;

while: KeyWhile expr '{' statementlist '}' { DEBUG("while"); };

identlist: Ident ',' identlist
        | Ident;

decl: type ':' identlist
    | storagequalifier type ':' identlist

definition: decl '=' expr { DEBUG("Definition"); };

storagequalifier: KeyGlobal
        | KeyStatic
        | KeyLocal;

assign: Ident '=' expr { DEBUG("Assignment"); }
      | boxaccess '=' expr 
      | boxselfaccess '=' expr ;

sign: KeySigned
    | KeyUnsigned;

typedef: KeyType type':' Ident;

scale: scale KeyShort
    | scale KeyHalf
    | scale KeyLong
    | scale KeyDouble
    | KeyShort
    | KeyHalf
    | KeyLong
    | KeyDouble;

typekind: Ident
    | KeyInt
    | KeyFloat;

type: typekind
    | scale typekind
    | sign typekind
    | sign scale typekind;

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