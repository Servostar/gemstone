%locations
%define parse.error verbose

%code requires {
    #include <sys/log.h>
    #include <ast/ast.h>
    #include <sys/col.h>
    #include <io/files.h>
    #include <glib.h>
    extern int yylineno;
    extern ModuleFile* current_file;

    int yyerror(const char*);

    extern char* buffer;
    extern int yylineno;
    
    extern int yylex();
    extern AST_NODE_PTR root;

    #define new_loc() new_location(yylloc.first_line, yylloc.first_column, yylloc.last_line, yylloc.last_column, current_file)
}

%union {
    char *string;
    AST_NODE_PTR node_ptr;
}

%type <node_ptr> operation
%type <node_ptr> boxaccess
%type <node_ptr> boxselfaccess
%type <node_ptr> statement
%type <node_ptr> statementlist
%type <node_ptr> assign
%type <node_ptr> oparith
%type <node_ptr> decl
%type <node_ptr> definition
%type <node_ptr> while
%type <node_ptr> funcall
%type <node_ptr> boxcall
%type <node_ptr> branchhalf
%type <node_ptr> branchfull
%type <node_ptr> branchelse
%type <node_ptr> branchelseif
%type <node_ptr> branchif
%type <node_ptr> type
%type <node_ptr> identlist
%type <node_ptr> storagequalifier
%type <node_ptr> typekind
%type <node_ptr> scale
%type <node_ptr> sign
%type <node_ptr> expr
%type <node_ptr> oplogic
%type <node_ptr> opbool
%type <node_ptr> opbit
%type <node_ptr> moduleimport
%type <node_ptr> moduleinclude
%type <node_ptr> programbody
%type <node_ptr> fundef
%type <node_ptr> fundecl
%type <node_ptr> procdecl
%type <node_ptr> procdef
%type <node_ptr> box
%type <node_ptr> typedef
%type <node_ptr> exprlist
%type <node_ptr> argumentlist
%type <node_ptr> paramlist
%type <node_ptr> params
%type <node_ptr> IOqualifyier
%type <node_ptr> paramdecl 
%type <node_ptr> boxbody
%type <node_ptr> boxcontent
%type <node_ptr> typecast
%type <node_ptr> reinterpretcast
%type <node_ptr> program
%type <node_ptr> storage_expr
%type <node_ptr> returnstmt


%token KeyInt
%token KeyFloat
%token KeySelf
%token KeyAs
%token KeyTo
%token <string> ValInt
%token <string> Ident
%token <string> ValFloat 
%token <string> ValStr
%token <string> ValChar
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
%token KeyInclude
%token KeySilent
%token KeyBox
%token FunTypeof
%token FunSizeof
%token FunFilename
%token FunFunname
%token FunLineno
%token FunExtsupport
%token Invalid
%token KeyReturn

/* Operator associativity */
/* Operators at lower line number have lower precedence */
/* Operators in same line have same precedence */
%right '='
%left OpOr
%left OpXor
%left OpAnd
%left OpBitor
%left OpBitxor
%left OpBitand
%left OpEquals '<' '>'
%left '+' '-'
%left '*' '/'
%left OpNot OpBitnot
%left KeyAs KeyTo
%left '(' ')' '[' ']'

%%
program: program programbody {AST_push_node(root, $2); 
                              }
       | programbody {AST_push_node(root, $1);};

programbody: moduleimport {$$ = $1;}
       | moduleinclude {$$ = $1;}
       | fundef{$$ = $1;}
       | fundecl{$$ = $1;}
       | procdecl{$$ = $1;}
       | procdef{$$ = $1;}
       | box{$$ = $1;}
       | definition{$$ = $1;}
       | decl{$$ = $1;}
       | typedef{$$ = $1;};



expr: ValFloat {$$ = AST_new_node(new_loc(), AST_Float, $1);}
    | ValInt    {$$ = AST_new_node(new_loc(), AST_Int, $1);}
    | ValChar   {$$ = AST_new_node(new_loc(), AST_Char, $1);}
    | ValMultistr   {$$ = AST_new_node(new_loc(), AST_String, $1);}
    | ValStr    {$$ = AST_new_node(new_loc(), AST_String, $1);}
    | Ident     {$$ = AST_new_node(new_loc(), AST_Ident, $1);}
    | operation {$$ = $1;}
    | boxaccess {$$ = $1;}
    | boxselfaccess{$$ = $1;}
    | typecast{$$ = $1;}
    | reinterpretcast{$$ = $1;}
    | '(' expr ')' {$$=$2;}
    | funcall {$$=$1;}
    | KeyRef Ident {AST_NODE_PTR addrof = AST_new_node(new_loc(), AST_AddressOf, NULL);
                                   AST_push_node(addrof, AST_new_node(new_loc(), AST_Ident, $2));
                                   $$ = addrof;}
    | expr '[' expr ']' {AST_NODE_PTR deref = AST_new_node(new_loc(), AST_Dereference, NULL);
                                   AST_push_node(deref, $1);
                                   AST_push_node(deref, $3);
                                   $$ = deref;};

exprlist: expr ',' exprlist {AST_push_node($3, $1);
                             $$ = $3;}
        | expr {AST_NODE_PTR list = AST_new_node(new_loc(), AST_ExprList, NULL);
                AST_push_node(list, $1);
                $$ = list;};

argumentlist: argumentlist '(' exprlist ')' {AST_push_node($1, $3);
                                             $$ = $1;}
    | '(' exprlist ')'{AST_NODE_PTR list = AST_new_node(new_loc(), AST_ArgList, NULL);
                AST_push_node(list, $2);
                $$ = list;}
    | argumentlist '(' ')'
    | '(' ')'{AST_NODE_PTR list = AST_new_node(new_loc(), AST_ArgList, NULL);
              $$ = list;};


procdef: KeyFun Ident paramlist '{' statementlist'}' {AST_NODE_PTR fun = AST_new_node(new_loc(), AST_ProcDef, NULL);
                                                     AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $2);
                                                     AST_push_node(fun, ident);
                                                     AST_push_node(fun, $3);
                                                     AST_push_node(fun, $5);
                                                     $$ = fun;
                                                            DEBUG("Function");}

procdecl: KeyFun Ident paramlist {AST_NODE_PTR fun = AST_new_node(new_loc(), AST_ProcDecl, NULL);
                                 AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $2);
                                 AST_push_node(fun, ident);
                                 AST_push_node(fun, $3);
                                 $$ = fun;
                                        DEBUG("Function");};

fundef: KeyFun type ':' Ident paramlist '{' statementlist'}' {AST_NODE_PTR fun = AST_new_node(new_loc(), AST_FunDef, NULL);
                                                     AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $4);
                                                     AST_push_node(fun, ident);
                                                     AST_push_node(fun, $2);
                                                     AST_push_node(fun, $5);
                                                     AST_push_node(fun, $7);
                                                     $$ = fun;
                                                            DEBUG("Function");}

fundecl: KeyFun type ':' Ident paramlist {AST_NODE_PTR fun = AST_new_node(new_loc(), AST_FunDecl, NULL);
                                 AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $4);
                                 AST_push_node(fun, ident);
                                 AST_push_node(fun, $2);
                                 AST_push_node(fun, $5);
                                 $$ = fun;
                                        DEBUG("Function");};



paramlist: paramlist '(' params ')' {AST_push_node($1, $3);
                                     $$ = $1;}
         |  paramlist '(' ')'{$$ = $1;}
         | '(' params ')' {AST_NODE_PTR list = AST_new_node(new_loc(), AST_List, NULL);
                           AST_push_node(list, $2);
                           $$ = list;}
         | '(' ')' {$$ = AST_new_node(new_loc(), AST_List, NULL);};

params: IOqualifyier paramdecl ',' params {AST_NODE_PTR parameter = AST_new_node(new_loc(), AST_Parameter, NULL);
                                AST_push_node(parameter, $1);
                                AST_push_node(parameter, $2);
                                AST_push_node($4, parameter);
                                $$ = $4;}
      | IOqualifyier paramdecl {AST_NODE_PTR list = AST_new_node(new_loc(), AST_ParamList, NULL);
                                AST_NODE_PTR parameter = AST_new_node(new_loc(), AST_Parameter, NULL);
                                AST_push_node(parameter, $1);
                                AST_push_node(parameter, $2);
                                AST_push_node(list, parameter);
                                $$ = list;};

IOqualifyier: KeyIn { AST_NODE_PTR in = AST_new_node(new_loc(), AST_Qualifyier, "in");
                      AST_NODE_PTR list = AST_new_node(new_loc(), AST_List, NULL);
                      AST_push_node(list, in);
                      $$ = list;}
            | KeyOut{ AST_NODE_PTR out = AST_new_node(new_loc(), AST_Qualifyier, "out");
                      AST_NODE_PTR list = AST_new_node(new_loc(), AST_List, NULL);
                      AST_push_node(list, out);
                      $$ = list;}
            | KeyIn KeyOut{ AST_NODE_PTR in = AST_new_node(new_loc(), AST_Qualifyier, "in");
                      AST_NODE_PTR out = AST_new_node(new_loc(), AST_Qualifyier, "out");
                      AST_NODE_PTR list = AST_new_node(new_loc(), AST_List, NULL);
                      AST_push_node(list, in);
                      AST_push_node(list, out);
                      $$ = list;}
            | KeyOut KeyIn{ AST_NODE_PTR in = AST_new_node(new_loc(), AST_Qualifyier, "in");
                      AST_NODE_PTR out = AST_new_node(new_loc(), AST_Qualifyier, "out");
                      AST_NODE_PTR list = AST_new_node(new_loc(), AST_List, NULL);
                      AST_push_node(list, in);
                      AST_push_node(list, out);
                      $$ = list;}
            | {$$ = AST_new_node(new_loc(), AST_List, NULL);};

paramdecl: type ':' Ident { AST_NODE_PTR paramdecl = AST_new_node(new_loc(), AST_ParamDecl, NULL);
                            AST_push_node(paramdecl, $1);
                            AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $3);
                            AST_push_node(paramdecl, ident);
                            $$ = paramdecl;
    DEBUG("Param-Declaration"); };

box: KeyType KeyBox ':' Ident '{' boxbody '}' {AST_NODE_PTR box = AST_new_node(new_loc(), AST_Box, NULL);
                                       AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $4);
                                       AST_push_node(box, ident);
                                       AST_push_node(box, $6);
                                       $$ = box; 
    DEBUG("Box"); }
   | KeyType KeyBox ':' Ident '{' '}' {AST_NODE_PTR box = AST_new_node(new_loc(), AST_Box, NULL);
                                       AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $4);
                                       AST_push_node(box, ident);
                                       $$ = box;};

boxbody: boxbody boxcontent {AST_push_node($1, $2);
                             $$ = $1;}
       | boxcontent {AST_NODE_PTR list = AST_new_node(new_loc(), AST_List, NULL);
                     AST_push_node(list, $1);
                     $$ = list;};

boxcontent: decl { $$ = $1;DEBUG("Box decl Content"); }
          | definition { $$ = $1;DEBUG("Box def Content"); }
          | fundef { $$ = $1;DEBUG("Box fun Content"); };

boxselfaccess: KeySelf '.' Ident {AST_NODE_PTR boxselfaccess = AST_new_node(new_loc(), AST_List, NULL);
                                      AST_NODE_PTR self = AST_new_node(new_loc(), AST_Ident, "self");
                                      AST_push_node(boxselfaccess, self);
                                      AST_NODE_PTR identlist = AST_new_node(new_loc(), AST_IdentList, NULL);
                                      AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $3);
                                      AST_push_node(identlist,ident);
                                      AST_push_node(boxselfaccess, identlist);
                                      $$ = boxselfaccess;}
             | KeySelf '.' boxaccess {AST_NODE_PTR boxselfaccess = AST_new_node(new_loc(), AST_List, NULL);
                                      AST_NODE_PTR self = AST_new_node(new_loc(), AST_Ident, "self");
                                      AST_push_node(boxselfaccess, self);
                                      AST_push_node(boxselfaccess, $3);
                                      $$ = boxselfaccess;};

boxaccess: Ident '.' Ident {AST_NODE_PTR identlist = AST_new_node(new_loc(), AST_IdentList, NULL);
                                      AST_NODE_PTR ident1 = AST_new_node(new_loc(), AST_Ident, $1);
                                      AST_NODE_PTR ident2 = AST_new_node(new_loc(), AST_Ident, $3);
                                      AST_push_node(identlist,ident1);
                                      AST_push_node(identlist,ident2);
                                      $$ = identlist;}
         | Ident '.' boxaccess {AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $1);
                                      AST_push_node($3,ident);
                                      $$ = $3;};

boxcall: boxaccess argumentlist {AST_NODE_PTR boxcall = AST_new_node(new_loc(), AST_Call, NULL);
                                 AST_push_node(boxcall, $1);
                                 AST_push_node(boxcall, $2);
                                 $$ = boxcall;}
       | boxselfaccess argumentlist {AST_NODE_PTR boxcall = AST_new_node(new_loc(), AST_Call, NULL);
                                 AST_push_node(boxcall, $1);
                                 AST_push_node(boxcall, $2);
                                 $$ = boxcall;};
                                 

typecast: expr KeyAs type %prec KeyAs  {AST_NODE_PTR cast = AST_new_node(new_loc(), AST_Typecast, NULL);
                                        AST_push_node(cast, $1);
                                        AST_push_node(cast, $3);
                                        $$ = cast;
                                         DEBUG("Type-Cast"); };

reinterpretcast: expr KeyTo type %prec KeyTo { AST_NODE_PTR cast = AST_new_node(new_loc(), AST_Transmute, NULL);
                                      AST_push_node(cast, $1);
                                        AST_push_node(cast, $3);
                                        $$ = cast;
                                    DEBUG("Reinterpret-Cast"); };


funcall: Ident argumentlist {AST_NODE_PTR funcall = AST_new_node(new_loc(), AST_Call, NULL);
                             AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $1);
                             AST_push_node(funcall, ident);
                             AST_push_node(funcall, $2);
                             $$ = funcall;
                             DEBUG("Function call"); };

moduleimport: KeyImport ValStr {$$ = AST_new_node(new_loc(), AST_Import, $2);
                                 DEBUG("Module-Import"); };

moduleinclude: KeyInclude ValStr {$$ = AST_new_node(new_loc(), AST_Include, $2);
                                 DEBUG("Module-Include"); };

statementlist: statementlist statement  {AST_push_node($1, $2);
                                        $$ = $1;}
    | statement {AST_NODE_PTR list = AST_new_node(new_loc(), AST_StmtList, NULL);
                 AST_push_node(list, $1);
        $$ = list;};

statement: assign {$$ = $1;}
        | decl {$$ = $1;}
        | definition {$$ = $1;}
        | while {$$ = $1;}
        | branchfull {$$ = $1;}
        | returnstmt {$$ = $1;}
        | funcall {$$ = $1;}
        | boxcall{$$ = $1;};

returnstmt: KeyReturn expr { AST_NODE_PTR return_stmt = AST_new_node(new_loc(), AST_Return, NULL);
                         AST_push_node(return_stmt, $2);
                          $$ = return_stmt; };

branchif: KeyIf expr '{' statementlist '}' { AST_NODE_PTR branch = AST_new_node(new_loc(), AST_If, NULL);
                                            AST_push_node(branch, $2);
                                            AST_push_node(branch, $4);
                                            $$ = branch; };

branchelse: KeyElse '{' statementlist '}' { AST_NODE_PTR branch = AST_new_node(new_loc(), AST_Else, NULL);
                                            AST_push_node(branch, $3);
                                            $$ = branch; };

branchelseif: KeyElse KeyIf expr '{' statementlist '}' { AST_NODE_PTR branch = AST_new_node(new_loc(), AST_IfElse, NULL);
                                            AST_push_node(branch, $3);
                                            AST_push_node(branch, $5);
                                            $$ = branch; };

branchfull:  branchhalf { $$ = $1;};
            |branchhalf branchelse {   AST_push_node($1 , $2);
                                $$ = $1; }
branchhalf:  branchif  { AST_NODE_PTR branch = AST_new_node(new_loc(), AST_Stmt, NULL);
                                AST_push_node(branch, $1);
                                $$ = branch; }
        | branchhalf branchelseif { AST_push_node($1 , $2);
                                $$ = $1; };


while: KeyWhile expr '{' statementlist '}' {AST_NODE_PTR whilenode = AST_new_node(new_loc(), AST_While, NULL);
                                AST_push_node(whilenode, $2);
                                AST_push_node(whilenode, $4);
                                $$ = whilenode;};

identlist: Ident ',' identlist {AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $1);
                                AST_push_node($3, ident);
                                $$ = $3;}
        | Ident {AST_NODE_PTR list = AST_new_node(new_loc(), AST_IdentList, NULL);
                 AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $1);
                 AST_push_node(list, ident);
                 $$ = list;};               

decl: type ':' identlist {AST_NODE_PTR decl = AST_new_node(new_loc(), AST_Decl, NULL);
                          AST_push_node(decl, $1);
                          AST_push_node(decl, $3);
                          $$ = decl;}
    | storagequalifier type ':' identlist {AST_NODE_PTR decl = AST_new_node(new_loc(), AST_Decl, NULL);
                                           AST_push_node(decl, $1);
                                           AST_push_node(decl, $2);
                                           AST_push_node(decl, $4);
                                           $$ = decl;};


definition: decl '=' expr { AST_NODE_PTR def = AST_new_node(new_loc(), AST_Def, NULL);
                            AST_push_node(def, $1);
                            AST_push_node(def, $3);
                            $$ = def;
    DEBUG("Definition"); };

storagequalifier: KeyGlobal {$$ = AST_new_node(new_loc(), AST_Storage, "global");}
        | KeyStatic {$$ = AST_new_node(new_loc(), AST_Storage, "static");}
        | KeyLocal {$$ = AST_new_node(new_loc(), AST_Storage, "local");};

assign: storage_expr '=' expr { AST_NODE_PTR assign = AST_new_node(new_loc(), AST_Assign, NULL);
                         AST_push_node(assign, $1);
                         AST_push_node(assign, $3);
                         $$ = assign; };

storage_expr: Ident { $$ = AST_new_node(new_loc(), AST_Ident, $1); }
    | boxaccess  { $$ = $1; }
    | boxselfaccess  { $$ = $1; }
    | storage_expr '[' expr ']' { AST_NODE_PTR deref = AST_new_node(new_loc(), AST_Dereference, NULL);
                                                                    AST_push_node(deref, $1);
                                                                    AST_push_node(deref, $3);
                                                                    $$ = deref; };

sign: KeySigned {$$ = AST_new_node(new_loc(), AST_Sign, "signed");}
    | KeyUnsigned{$$ = AST_new_node(new_loc(), AST_Sign, "unsigned");};

typedef: KeyType type':' Ident {AST_NODE_PTR typeDef = AST_new_node(new_loc(), AST_Typedef, NULL);
                                AST_push_node(typeDef, $2);
                                AST_NODE_PTR ident = AST_new_node(new_loc(), AST_Ident, $4);
                                AST_push_node(typeDef, ident);
                                $$ = typeDef;};

scale: scale KeyShort {AST_NODE_PTR shortnode = AST_new_node(new_loc(), AST_Scale, "short");
                       AST_push_node($1, shortnode);
                       $$ = $1;}
    | scale KeyHalf {AST_NODE_PTR shortnode = AST_new_node(new_loc(), AST_Scale, "half");
                       AST_push_node($1, shortnode);
                       $$ = $1;}
    | scale KeyLong {AST_NODE_PTR shortnode = AST_new_node(new_loc(), AST_Scale, "long");
                       AST_push_node($1, shortnode);
                       $$ = $1;}
    | scale KeyDouble {AST_NODE_PTR shortnode = AST_new_node(new_loc(), AST_Scale, "double");
                       AST_push_node($1, shortnode);
                       $$ = $1;}
    | KeyShort {AST_NODE_PTR scale = AST_new_node(new_loc(), AST_List, NULL);
                AST_NODE_PTR shortnode = AST_new_node(new_loc(), AST_Scale, "short");
                AST_push_node(scale, shortnode);
                $$ = scale;}
    | KeyHalf {AST_NODE_PTR scale = AST_new_node(new_loc(), AST_List, NULL);
                AST_NODE_PTR shortnode = AST_new_node(new_loc(), AST_Scale, "half");
                AST_push_node(scale, shortnode);
                $$ = scale;}
    | KeyLong {AST_NODE_PTR scale = AST_new_node(new_loc(), AST_List, NULL);
                AST_NODE_PTR shortnode = AST_new_node(new_loc(), AST_Scale, "long");
                AST_push_node(scale, shortnode);
                $$ = scale;}
    | KeyDouble {AST_NODE_PTR scale = AST_new_node(new_loc(), AST_List, NULL);
                AST_NODE_PTR shortnode = AST_new_node(new_loc(), AST_Scale, "double");
                AST_push_node(scale, shortnode);
                $$ = scale;};

typekind: Ident {$$ = AST_new_node(new_loc(), AST_Typekind, $1);}
    | KeyInt {$$ = AST_new_node(new_loc(), AST_Typekind, "int");}
    | KeyFloat {$$ = AST_new_node(new_loc(), AST_Typekind, "float");};

type: typekind {AST_NODE_PTR type = AST_new_node(new_loc(), AST_Type, NULL);
                AST_push_node(type, $1);
                $$ = type;}
    | scale typekind {AST_NODE_PTR type = AST_new_node(new_loc(), AST_Type, NULL);
                AST_push_node(type, $1);
                AST_push_node(type, $2);
                $$ = type;}
    | sign typekind {AST_NODE_PTR type = AST_new_node(new_loc(), AST_Type, NULL);
                AST_push_node(type, $1);
                AST_push_node(type, $2);
                $$ = type;}
    | sign scale typekind {AST_NODE_PTR type = AST_new_node(new_loc(), AST_Type, NULL);
                AST_push_node(type, $1);
                AST_push_node(type, $2);
                AST_push_node(type, $3);
                $$ = type;}
    | KeyRef type {AST_NODE_PTR reftype = AST_new_node(new_loc(), AST_Reference, NULL);
               AST_push_node(reftype, $2);
               $$ = reftype; };

operation: oparith {$$ = $1;}
    | oplogic {$$ = $1;}
    | opbool {$$ = $1;}
    | opbit {$$ = $1;};

oparith: expr '+' expr {AST_NODE_PTR add = AST_new_node(new_loc(), AST_Add, NULL);
                        AST_push_node(add, $1);
                        AST_push_node(add, $3);
                        $$ = add;}
    | expr '-' expr    {AST_NODE_PTR subtract = AST_new_node(new_loc(), AST_Sub, NULL);
                        AST_push_node(subtract, $1);
                        AST_push_node(subtract, $3);
                        $$ = subtract;}
    | expr '*' expr     {AST_NODE_PTR mul = AST_new_node(new_loc(), AST_Mul, NULL);
                        AST_push_node(mul, $1);
                        AST_push_node(mul, $3);
                        $$ = mul;}
    | expr '/' expr     {AST_NODE_PTR div = AST_new_node(new_loc(), AST_Div, NULL);
                        AST_push_node(div, $1);
                        AST_push_node(div, $3);
                        $$ = div;}
    | '-' expr %prec '*'{AST_NODE_PTR negator = AST_new_node(new_loc(), AST_Negate, NULL);
                        AST_push_node(negator, $2);
                        $$ = negator;};

oplogic: expr OpEquals expr {AST_NODE_PTR equals = AST_new_node(new_loc(), AST_Eq, NULL);
                             AST_push_node(equals, $1);
                             AST_push_node(equals, $3);
                             $$ = equals;}
    | expr '<' expr {AST_NODE_PTR less = AST_new_node(new_loc(), AST_Less, NULL);
                             AST_push_node(less, $1);
                             AST_push_node(less, $3);
                             $$ = less;}
    | expr '>' expr{AST_NODE_PTR greater = AST_new_node(new_loc(), AST_Greater, NULL);
                             AST_push_node(greater, $1);
                             AST_push_node(greater, $3);
                             $$ = greater;};

opbool: expr OpAnd expr {AST_NODE_PTR and = AST_new_node(new_loc(), AST_BoolAnd, NULL);
                             AST_push_node(and, $1);
                             AST_push_node(and, $3);
                             $$ = and;}
    | expr OpOr expr{AST_NODE_PTR or = AST_new_node(new_loc(), AST_BoolOr, NULL);
                             AST_push_node(or, $1);
                             AST_push_node(or, $3);
                             $$ = or;}
    | expr OpXor expr{AST_NODE_PTR xor = AST_new_node(new_loc(), AST_BoolXor, NULL);
                             AST_push_node(xor, $1);
                             AST_push_node(xor, $3);
                             $$ = xor;}
    | OpNot expr %prec OpAnd{AST_NODE_PTR not = AST_new_node(new_loc(), AST_BoolNot, NULL);
                             AST_push_node(not, $2);
                             $$ = not;};

opbit: expr OpBitand expr {AST_NODE_PTR and = AST_new_node(new_loc(), AST_BitAnd, NULL);
                             AST_push_node(and, $1);
                             AST_push_node(and, $3);
                             $$ = and;}
    | expr OpBitor expr{AST_NODE_PTR or = AST_new_node(new_loc(), AST_BitOr, NULL);
                             AST_push_node(or, $1);
                             AST_push_node(or, $3);
                             $$ = or;}
    | expr OpBitxor expr{AST_NODE_PTR xor = AST_new_node(new_loc(), AST_BitXor, NULL);
                             AST_push_node(xor, $1);
                             AST_push_node(xor, $3);
                             $$ = xor;}
    | OpBitnot expr %prec OpBitand{AST_NODE_PTR not = AST_new_node(new_loc(), AST_BitNot, NULL);
                             AST_push_node(not, $2);
                             $$ = not;};
%%

int yyerror(const char *s) {
    TokenLocation location = new_loc();
    print_diagnostic(&location, Error, s);
    return 0;
}