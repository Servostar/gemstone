%option noyywrap
%{
    #include <yacc/parser.tab.h>
    #include <sys/log.h>
    #include <lex/util.h>

    int yyLineNumber = 1;
  
    int yylex();

    extern int yyerror(const char* s);
  
    #define YY_USER_ACTION beginToken(yytext);

    #define YY_INPUT(buf,result,max_size)  {\
        result = nextChar(buf); \
        if (  result <= 0  ) \
          result = YY_NULL; \
        }
%}

/* disable the following functions */
/* to avoid failing code check */
%option nounput
%option noinput

%%
"\n"    yyLineNumber++;

#.* ;

":" {DEBUG("\"%s\" tokenized with \':\'", yytext); return(':');};
"=" {DEBUG("\"%s\" tokenized with \'=\'", yytext); return('=');};
"+" {DEBUG("\"%s\" tokenized with \'+\'", yytext); return('+');};
"-" {DEBUG("\"%s\" tokenized with \'-\'", yytext); return('-');};
"*" {DEBUG("\"%s\" tokenized with \'*\'", yytext); return('*');};
"/" {DEBUG("\"%s\" tokenized with \'/\'", yytext); return('/');};
"," {DEBUG("\"%s\" tokenized with \',\'", yytext); return(',');};
";" {DEBUG("\"%s\" tokenized with \';\'", yytext); return(';');};
"." {DEBUG("\"%s\" tokenized with \'.\'", yytext); return('.');};

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
"self" {DEBUG("\"%s\" tokenized with \'KeySelf\'", yytext); return(KeySelf);};
"as" {DEBUG("\"%s\" tokenized with \'KeyAs'", yytext); return (KeyAs);};
"to" {DEBUG("\"%s\" tokenized with \'KeyTo'", yytext); return (KeyTo);};
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

[0-9]+    {DEBUG("\"%s\" tokenized with \'ValInt\'", yytext); yylval.string = lex_cached_strdup(yytext); return(ValInt); };
[0-9]*\.[0-9]+ {DEBUG("\"%s\" tokenized with \'ValFloat\'", yytext); yylval.string = lex_cached_strdup(yytext); return(ValFloat);};
[a-zA-Z_0-9]+ {DEBUG("\"%s\" tokenized with \'Ident\'", yytext); yylval.string = lex_cached_strdup(yytext); return(Ident); };

\"([^\"\n])*\" {
    yytext = yytext +1;
    yytext[yyleng - 2] = 0;
    
    DEBUG("\"%s\" tokenized with \'ValStr\'", yytext); yylval.string = lex_cached_strdup(yytext); return(ValStr);};
\"\"\"[^\"]*\"\"\" {
    yytext = yytext +3;
    yytext[yyleng - 4] = 0;

    DEBUG("\"%s\" tokenized with \'ValMultistr\'", yytext); yylval.string = lex_cached_strdup(yytext); return(ValMultistr);};
[ \r\t] { /* ignore whitespace */ };
. { return yytext[0]; /* passthrough unknown token, let parser handle the error */ };
%%
