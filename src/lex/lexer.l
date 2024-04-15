%option noyywrap
%{
    #include <yacc/parser.tab.h>

    int yyLineNumber = 1;
    int yylex();


%}


/* disable the following functions */
/* to avoid failing code check */
%option nounput
%option noinput


%%
"\n"    yyLineNumber++;


":" return(':');
"=" return('=');
"|" return('|');
"!" return('!');
"+"  return('+');
"-"  return('-');
"*"  return('*');
"/"  return('/');


"("  return('(');
")"  return(')');
"["  return('[');
"]"  return(']');
"{"  return('{');
"}"  return('}');
">"  return('>');
"<"  return('<');


"int" return(KeyInt);
"float" return(KeyFloat);

"as" return (KeyAs);


"short"  return(KeyShort);
"long" return(KeyLong);
"half"  return(KeyHalf);
"double" return(KeyDouble); 
"signed" return(KeySigned);
"unsigned" return(Keyunsigned);

"ref" return(KeyRef);
"type" return(KeyType);
"local" return(KeyLocal);
"global" return(KeyGlobal);
"static" return(KeyStatic);

"if" return(KeyIf);
"else" return(KeyElse);

"while" return(KeyWhile);

"in" return(KeyIn);
"out" return(KeyOut);
"fun" return(KeyFun);
"," return(',');
";"  return(';');
"==" return(OpEquals);
"&&" return(OpAnd);
"||" return(OpOr);
"!!" return(OpNot);
"^^" return(OpXor);
"&" return('&');
"^" return('^');
"." return('.');


"import" return(KeyImport);
"silent" return(KeySilent);
"box" return(KeyBox);
"#" return('#');
"typeof" return(FunTypeof);
"sizeof" return(FunSizeof);
"filename" return(FunFilename);
"funname" return(FunFunname);
"lineno" return(FunLineno);
"extsupport" return(FunExtsupport);



[a-zA-Z_]+ { yylval.string = strdup(yytext); return(Ident); };
[0-9]+    { yylval.num = atoi(yytext); return(ValInt); };
\"([^\"\n])*\" {yylval.string = strdup(yytext); return(ValStr);};
\"\"\"([^\"\n]|\\\n)*\"\"\" {yylval.string = strdup(yytext); return(ValMultistr);};
.;
%%
