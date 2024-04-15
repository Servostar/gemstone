%{
     #include <stdio.h>
    extern int yylineno;

    int yyerror(char*);

    extern int yylex();

    int array[256];
%}

%union {
    char *string;
    int num;
}


%token KeyInt
%token KeyFloat

%token KeyAs
%token <num> ValInt
%token <string> Ident

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


%token KeyImport
%token KeySilent
%token KeyBox
%token FunTypeof
%token FunSizeof
%token FunFilename
%token FunFunname
%token FunLineno
%token FunExtsupport
%token ValStr
%token ValMultistr


%%
program: decllist;

decllist: decl
    | decllist decl;

decl: '=' {printf("=\n"); };
    | ':' {printf(":\n"); };
    | '|' {printf("|\n"); };
    | '(' {printf("(\n"); };
    | ')' {printf(")\n"); };
    | '[' {printf("[\n"); };
    | ']' {printf("]\n"); };
    | '{' {printf("{\n"); };
    | '}' {printf("}\n"); }; 
    | '!' {printf("!\n"); }; 
    
    | '+' {printf("+\n"); }; 
    | '-' {printf("-\n"); }; 
    | '*' {printf("*\n"); }; 
    | '/' {printf("/\n"); }; 

    | '&' {printf("&\n"); }; 
    | '^' {printf("^\n"); }; 
    | '.' {printf(".\n"); }; 
    | ',' {printf(",\n"); }; 
    | ';' {printf(";\n"); }; 
    | '#' {printf("#\n"); }; 



    | Ident {printf("Ident\n"); };
    | ValInt {printf("ValInt\n"); };
    | KeyInt {printf("KeyInt\n"); };
    | KeyFloat {printf("KeyFloat\n"); };
    | KeyAs {printf("KeyAs\n"); };

    | KeyShort {printf("KeyShort\n"); };
    | KeyLong {printf("KeyLong\n"); };
    | KeyHalf {printf("KeyHalf\n"); };
    | KeyDouble {printf("KeyDouble\n"); };
    | KeySigned {printf("KeySigned\n"); };
    | Keyunsigned {printf("Keyunsigned\n"); };

    | KeyRef {printf("KeyRef\n"); };
    | KeyType {printf("KeyType\n"); };
    | KeyLocal {printf("KeyLocal\n"); };
    | KeyGlobal {printf("KeyGlobal\n"); };
    | KeyStatic {printf("KeyStatic\n"); };

    | KeyIf {printf("KeyIf\n"); };
    | KeyElse {printf("KeyElse\n"); };

    | KeyWhile {printf("KeyWhile\n"); };

    | KeyIn {printf("KeyIn\n"); };
    | KeyOut {printf("KeyOut\n"); };
    | KeyFun {printf("KeyFun\n"); };
    | OpEquals {printf("OpEquals\n"); };
    | OpAnd {printf("OpAnd\n"); };
    | OpOr {printf("OpOr\n"); };
    | OpNot {printf("OpNot\n"); };
    | OpXor {printf("OpXor\n"); };

    | KeyImport {printf("KeyImport\n"); };
    | KeySilent {printf("KeySilent\n"); };
    | KeyBox {printf("KeyBox\n"); };
    | FunTypeof {printf("FunTypeof\n"); };
    | FunSizeof {printf("FunSizeof\n"); };
    | FunFilename {printf("FunFilename\n"); };
    | FunLineno {printf("FunLineno\n"); };
    | FunExtsupport {printf("FunExtsupport\n"); };
    | FunFunname {printf("FunFunname\n"); };

    | ValStr {printf("ValStr\n"); };
    | ValMultistr {printf("ValMultistr\n"); };


%%



int yyerror(char *s) {
    return 0;
}