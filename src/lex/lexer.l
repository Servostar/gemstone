%option noyywrap
%{
    #include <yacc/parser.tab.h>
    #include <sys/log.h>

    int yyLineNumber = 1;
    int yylex();
%}

/* disable the following functions */
/* to avoid failing code check */
%option nounput
%option noinput

%%
"\n"    yyLineNumber++;

":" {DEBUG("\"%s\" tokenized with \':\'", yytext); return(':');};
"=" {DEBUG("\"%s\" tokenized with \'=\'", yytext); return('=');};
"+" {DEBUG("\"%s\" tokenized with \'+\'", yytext); return('+');};
"-" {DEBUG("\"%s\" tokenized with \'-\'", yytext); return('-');};
"*" {DEBUG("\"%s\" tokenized with \'*\'", yytext); return('*');};
"/" {DEBUG("\"%s\" tokenized with \'/\'", yytext); return('/');};
"," {DEBUG("\"%s\" tokenized with \',\'", yytext); return(',');};
";" {DEBUG("\"%s\" tokenized with \';\'", yytext); return(';');};
"." {DEBUG("\"%s\" tokenized with \'.\'", yytext); return('.');};
"#" {DEBUG("\"%s\" tokenized with \'#\'", yytext); return('#');};

"(" {DEBUG("\"%s\" tokenized with \'(\'", yytext); return('(');};
")" {DEBUG("\"%s\" tokenized with \')\'", yytext); return(')');};
"[" {DEBUG("\"%s\" tokenized with \'[\'", yytext); return('[');};
"]" {DEBUG("\"%s\" tokenized with \']\'", yytext); return(']');};
"{" {DEBUG("\"%s\" tokenized with \'{\'", yytext); return('{');};
"}" {DEBUG("\"%s\" tokenized with \'}\'", yytext); return('}');};
">" {DEBUG("\"%s\" tokenized with \'>\'", yytext); return('>');};
"<" {DEBUG("\"%s\" tokenized with \'<\'", yytext); return('<');};

"int" {DEBUG("\"%s\" tokenized with \'KeyInt'", yytext); return(KeyInt);};
"float" {DEBUG("\"%s\" tokenized with \'KeyFloat\'", yytext); return(KeyFloat);};
"as" {DEBUG("\"%s\" tokenized with \'KeyAs'", yytext); return (KeyAs);};
"short" {DEBUG("\"%s\" tokenized with \'KeyShort\'", yytext); return(KeyShort);};
"long" {DEBUG("\"%s\" tokenized with \'KeyLong\'", yytext); return(KeyLong);};
"half" {DEBUG("\"%s\" tokenized with \'KeyHalf\'", yytext); return(KeyHalf);};
"double" {DEBUG("\"%s\" tokenized with \'KeyDouble\'", yytext); return(KeyDouble);}; 
"signed" {DEBUG("\"%s\" tokenized with \'KeySigned\'", yytext); return(KeySigned);};
"unsigned" {DEBUG("\"%s\" tokenized with \'KeyUnsigned\'", yytext); return(KeyUnsigned);};
"ref" {DEBUG("\"%s\" tokenized with \'KeyRef\'", yytext); return(KeyRef);};
"type" {DEBUG("\"%s\" tokenized with \'KeyType\'", yytext); return(KeyType);};
"local" {DEBUG("\"%s\" tokenized with \'KeyLocal\'", yytext); return(KeyLocal);};
"global" {DEBUG("\"%s\" tokenized with \'KeyGlobal\'", yytext); return(KeyGlobal);};
"static" {DEBUG("\"%s\" tokenized with \'KeyStatic\'", yytext); return(KeyStatic);};
"if" {DEBUG("\"%s\" tokenized with \'KeyIf\'", yytext); return(KeyIf);};
"else" {DEBUG("\"%s\" tokenized with \'KeyElse\'", yytext); return(KeyElse);};
"while" {DEBUG("\"%s\" tokenized with \'KeyWhile\'", yytext); return(KeyWhile);};
"in" {DEBUG("\"%s\" tokenized with \'KeyIn\'", yytext); return(KeyIn);};
"out" {DEBUG("\"%s\" tokenized with \'KeyOut\'", yytext); return(KeyOut);};
"fun" {DEBUG("\"%s\" tokenized with \'KeyFun\'", yytext); return(KeyFun);};
"==" {DEBUG("\"%s\" tokenized with \'OpEquals\'", yytext); return(OpEquals);};
"&&" {DEBUG("\"%s\" tokenized with \'OpAnd\'", yytext); return(OpAnd);};
"||" {DEBUG("\"%s\" tokenized with \'OpOr\'", yytext); return(OpOr);};
"!!" {DEBUG("\"%s\" tokenized with \'OpNot\'", yytext); return(OpNot);};
"^^" {DEBUG("\"%s\" tokenized with \'OpXor\'", yytext); return(OpXor);};
"&" {DEBUG("\"%s\" tokenized with \'OpBitand\'", yytext); return(OpBitand);};
"|" {DEBUG("\"%s\" tokenized with \'OpBitor\'", yytext); return(OpBitor);};
"!" {DEBUG("\"%s\" tokenized with \'OpBitnot\'", yytext); return(OpBitnot);};
"^" {DEBUG("\"%s\" tokenized with \'OpBitxor\'", yytext); return(OpBitxor);};
"import" {DEBUG("\"%s\" tokenized with \'KeyImport\'", yytext); return(KeyImport);};
"silent" {DEBUG("\"%s\" tokenized with \'KeySilent\'", yytext); return(KeySilent);};
"box" {DEBUG("\"%s\" tokenized with \'KeyBox\'", yytext); return(KeyBox);};
"typeof" {DEBUG("\"%s\" tokenized with \'FunTypeof\'", yytext); return(FunTypeof);};
"sizeof" {DEBUG("\"%s\" tokenized with \'FunSizeof\'", yytext); return(FunSizeof);};
"filename" {DEBUG("\"%s\" tokenized with \'FunFilename\'", yytext); return(FunFilename);};
"funname" {DEBUG("\"%s\" tokenized with \'FunFunname\'", yytext); return(FunFunname);};
"lineno" {DEBUG("\"%s\" tokenized with \'FunLineno\'", yytext); return(FunLineno);};
"extsupport" {DEBUG("\"%s\" tokenized with \'FunExtsupport\'", yytext); return(FunExtsupport);};

[0-9]+    {DEBUG("\"%s\" tokenized with \'ValInt\'", yytext); yylval.string = strdup(yytext); return(ValInt); };
[0-9]*\.[0-9]+ {DEBUG("\"%s\" tokenized with \'ValFloat\'", yytext); yylval.string = strdup(yytext); return(ValFloat);};
[a-zA-Z_0-9]+ {DEBUG("\"%s\" tokenized with \'Ident\'", yytext); yylval.string = strdup(yytext); return(Ident); };
\"([^\"\n])*\" {DEBUG("\"%s\" tokenized with \'ValStr\'", yytext); yylval.string = strdup(yytext); return(ValStr);};
\"\"\"([^\"\n]|\\\n)*\"\"\" {DEBUG("\"%s\" tokenized with \'ValMultistr\'", yytext); yylval.string = strdup(yytext); return(ValMultistr);};
.;
%%
