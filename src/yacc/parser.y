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
    
%%



int yyerror(char *s) {
    return 0;
}