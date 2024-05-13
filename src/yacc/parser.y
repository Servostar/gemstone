%{
    #include <sys/log.h>
    #include <ast/ast.h>
    extern int yylineno;

    int yyerror(char*);

    extern int yylex();
%}

%union {
    char *string;
}

%type <AST_NODE_PTR> expr
%type <AST_NODE_PTR> operation
%type <AST_NODE_PTR> boxaccess
%type <AST_NODE_PTR> boxselfaccess
%type <AST_NODE_PTR> statement
%type <AST_NODE_PTR> statementlist
%type <AST_NODE_PTR> assign
%type <AST_NODE_PTR> oparith
%type <AST_NODE_PTR> decl
%type <AST_NODE_PTR> definition
%type <AST_NODE_PTR> while
%type <AST_NODE_PTR> branch
%type <AST_NODE_PTR> funcall
%type <AST_NODE_PTR> boxcall
%type <AST_NODE_PTR> branchelseifs
%type <AST_NODE_PTR> branchelse
%type <AST_NODE_PTR> branchelseif
%type <AST_NODE_PTR> branchif
%type <AST_NODE_PTR> type
%type <AST_NODE_PTR> identlist
%type <AST_NODE_PTR> storagequalifier
%type <AST_NODE_PTR> typekind
%type <AST_NODE_PTR> scale
%type <AST_NODE_PTR> sign
%type <AST_NODE_PTR> oplogic
%type <AST_NODE_PTR> opbool
%type <AST_NODE_PTR> opbit


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

%%
program: program programbody
       | programbody {AST_NODE_PTR program = AST_new_node(AST_Module, NULL);
                      AST_fprint_graphviz(stdout, program);  };

programbody: moduleimport
       | fundef
       | box
       | definition
       | decl
       | typedef;



expr: ValFloat {$$ = AST_new_node{AST_Float, $1};}
    | ValInt    {$$ = AST_new_node{AST_Int, $1};}
    | ValMultistr   {$$ = AST_new_node{AST_String, $1};}
    | ValStr    {$$ = AST_new_node{AST_String, $1};}
    | Ident     {$$ = AST_new_node{AST_Ident, $1};}
    | operation {$$ = $1;}
    | boxaccess {$$ = $1;}
    | boxselfaccess{$$ = $1;};

exprlist: expr ',' exprlist
        | expr ;

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

boxselfaccess: KeySelf '.' Ident {$$ = AST_new_node(AST_Call, NULL);}
             | KeySelf '.' boxaccess {$$ = AST_new_node(AST_Call, NULL);};

boxaccess: Ident '.' Ident {$$ = AST_new_node(AST_Call, NULL);}
         | Ident '.' boxaccess {$$ = AST_new_node(AST_Ident, $1);};

boxcall: boxaccess argumentlist
       | boxselfaccess argumentlist;

funcall: Ident argumentlist { DEBUG("Function call"); };

moduleimport: KeyImport ValStr { DEBUG("Module-Import"); };

statementlist: statement statementlist
    | statement {$$ = $1;};

statement: assign {$$ = $1;}
        | decl {$$ = $1;}
        | definition {$$ = $1;}
        | while {$$ = $1;}
        | branch {$$ = $1;}
        | funcall {$$ = $1;}
        | boxcall{$$ = $1;};

branchif: KeyIf expr '{' statementlist '}' { DEBUG("if"); };
branchelse: KeyElse '{' statementlist '}' { DEBUG("if-else"); };
branchelseif: KeyElse KeyIf expr '{' statementlist '}' { DEBUG("else-if"); };

branchelseifs: branchelseifs branchelseif {AST_NODE_PTR ifelse = AST_new_node(AST_IfElse, NULL);
                                           AST_push_node(ifelse, $1);
                                           AST_push_node(ifelse, $2);
                                           $$ = ifelse;}
    | branchelseif {$$ = $1;};

branch: branchif branchelseifs {AST_NODE_PTR branch = AST_new_node(AST_Stmt, NULL);
                                AST_push_node(branch, $1);
                                AST_push_node(branch, $2);
                                $$ = branch;}

    | branchif branchelseifs branchelse { AST_NODE_PTR branch = AST_new_node(AST_IF, NULL);};

while: KeyWhile expr '{' statementlist '}' { DEBUG("while"); };

identlist: Ident ',' identlist {AST_push_node($3, $1);
                                $$ = $3;}
        | Ident {AST_NODE_PTR list = AST_new_node(AST_List, NULL);
                 AST_push_node(list, $1);
                 $$ = list;};               

decl: type ':' identlist {AST_NODE_PTR decl = AST_new_node(AST_Decl, NULL);
                          AST_push_node(decl, $1);
                          AST_push_node(decl, $3);
                          $$ = decl;}
    | storagequalifier type ':' identlist {AST_NODE_PTR decl = AST_new_node(AST_Decl, NULL);
                                           AST_push_node(decl, $1);
                                           AST_push_node(decl, $2);
                                           AST_push_node(decl, $4);
                                           $$ = decl;}


definition: decl '=' expr { DEBUG("Definition"); };

storagequalifier: KeyGlobal {$$ = AST_new_node(AST_Storage, "global");}
        | KeyStatic {$$ = AST_new_node(AST_Storage, "static");}
        | KeyLocal {$$ = AST_new_node(AST_Storage, "local");};

assign: Ident '=' expr { AST_NODE_PTR assign = AST_new_node(AST_Assign, NULL);
                         AST_NODE_PTR ident = AST_new_node(AST_Ident, $1);
                         AST_push_node(assign, ident);
                         AST_push_node(assign, $3);
                         $$ = assign;
    DEBUG("Assignment"); }
      | boxaccess '=' expr 
      | boxselfaccess '=' expr ;

sign: KeySigned {$$ = AST_new_node(AST_Sign, "signed");}
    | KeyUnsigned{$$ = AST_new_node(AST_Sign, "unsigned");};

typedef: KeyType type':' Ident;

scale: scale KeyShort {AST_NODE_PTR shortnode = AST_new_node(AST_Scale, "short");
                       AST_push_node($1, shortnode);
                       $$ = $1;}
    | scale KeyHalf {AST_NODE_PTR shortnode = AST_new_node(AST_Scale, "half");
                       AST_push_node($1, shortnode);
                       $$ = $1;}
    | scale KeyLong {AST_NODE_PTR shortnode = AST_new_node(AST_Scale, "long");
                       AST_push_node($1, shortnode);
                       $$ = $1;}
    | scale KeyDouble {AST_NODE_PTR shortnode = AST_new_node(AST_Scale, "double");
                       AST_push_node($1, shortnode);
                       $$ = $1;}
    | KeyShort {AST_NODE_PTR scale = AST_new_node(AST_List, NULL);
                AST_NODE_PTR shortnode = AST_new_node(AST_Scale, "short");
                AST_push_node(scale, shortnode);
                $$ = scale;}
    | KeyHalf {AST_NODE_PTR scale = AST_new_node(AST_List, NULL);
                AST_NODE_PTR shortnode = AST_new_node(AST_Scale, "half");
                AST_push_node(scale, shortnode);
                $$ = scale;}
    | KeyLong {AST_NODE_PTR scale = AST_new_node(AST_List, NULL);
                AST_NODE_PTR shortnode = AST_new_node(AST_Scale, "long");
                AST_push_node(scale, shortnode);
                $$ = scale;}
    | KeyDouble {AST_NODE_PTR scale = AST_new_node(AST_List, NULL);
                AST_NODE_PTR shortnode = AST_new_node(AST_Scale, "double");
                AST_push_node(scale, shortnode);
                $$ = scale;};

typekind: Ident {$$ = AST_new_node(AST_Typekind, $1);}
    | KeyInt {$$ = AST_new_node(AST_Typekind, "int");}
    | KeyFloat {$$ = AST_new_node(AST_Typekind, "float");};

type: typekind {AST_NODE_PTR type = AST_new_node(AST_type, NULL);
                AST_push_node(type, $1);
                $$ = type;}
    | scale typekind {AST_NODE_PTR type = AST_new_node(AST_type, NULL);
                AST_push_node(type, $1);
                AST_push_node(type, $2);
                $$ = type;}
    | sign typekind {AST_NODE_PTR type = AST_new_node(AST_type, NULL);
                AST_push_node(type, $1);
                AST_push_node(type, $2);
                $$ = type;}
    | sign scale typekind {AST_NODE_PTR type = AST_new_node(AST_type, NULL);
                AST_push_node(type, $1);
                AST_push_node(type, $2);
                AST_push_node(type, $3);
                $$ = type;};

operation: oparith {$$ = $1;}
    | oplogic {$$ = $1;}
    | opbool {$$ = $1;}
    | opbit {$$ = $1;};

oparith: expr '+' expr {AST_NODE_PTR add = AST_new_node{AST_Add, NULL};
                        AST_push_node(add, $1);
                        AST_push_node(add, $3);
                        $$ = add;}
    | expr '-' expr    {AST_NODE_PTR subtract = AST_new_node{AST_Sub, NULL};
                        AST_push_node(subtract, $1);
                        AST_push_node(subtract, $3);
                        $$ = subtract;}
    | expr '*' expr     {AST_NODE_PTR mul = AST_new_node{AST_Mul, NULL};
                        AST_push_node(mul, $1);
                        AST_push_node(mul, $3);
                        $$ = mul;}
    | expr '/' expr     {AST_NODE_PTR div = AST_new_node{AST_Div, NULL};
                        AST_push_node(div, $1);
                        AST_push_node(div, $3);
                        $$ = div;}
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