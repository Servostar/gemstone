%locations
%define parse.error verbose

%{
    #include <sys/log.h>
    #include <sys/col.h>

    int yyerror(char*);

    extern char* buffer;
    extern int yylineno;
    
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
%token Invalid

/* Operator associativity */
%right '='
%left '+' '-' '*' '/'
%left OpEquals OpNot '<' '>'
%left OpAnd OpOr OpXor
%left OpBitand OpBitor OpBitxor OpBitnot

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
    | boxselfaccess;

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

 
const char* ERROR = "error";
const char* WARNING = "warning";
const char* NOTE = "note";

int print_message(const char* kind, char* message) {
    // number of characters written
    int char_count = 0;
    // highlight to use
    char* HIGHLIGHT = CYAN;

    // convert message kind into color
    if (kind == ERROR) {
        HIGHLIGHT = RED;
    } else if (kind == WARNING) {
        HIGHLIGHT = YELLOW;
    }

    // print message
    char_count += printf("%sfilename:%d:%d%s:%s%s %s: %s%s\n", BOLD, yylloc.first_line, yylloc.first_column, RESET, HIGHLIGHT, BOLD, kind, RESET, message);
    
    // print line in which error occurred
    
    char_count += printf(" %4d | ", yylloc.first_line);

    for (int i = 0; i < yylloc.first_column - 1; i++) {
        if (buffer[i] == '\n') {
            break;
        }
        printf("%c", buffer[i]);
    }

    char_count += printf("%s%s", BOLD, HIGHLIGHT);

    for (int i = yylloc.first_column - 1; i < yylloc.last_column; i++) {
        if (buffer[i] == '\n') {
            break;
        }
        char_count += printf("%c", buffer[i]);
    }

    char_count += printf("%s", RESET);

    for (int i = yylloc.last_column; buffer[i] != '\0' && buffer[i] != '\n'; i++) {
        printf("%c", buffer[i]);
    }

    char_count += printf("\n      | ");

    for (int i = 0; i < yylloc.first_column - 1; i++) {
        char_count += printf(" ");
    }

    char_count += printf("%s^", HIGHLIGHT);

    for (int i = 0; i < yylloc.last_column - yylloc.first_column; i++) {
        printf("~");
    }
    
    char_count += printf("%s\n\n", RESET);
}

int yyerror(char *s) {
    print_message(ERROR, s);
    return 0;
}