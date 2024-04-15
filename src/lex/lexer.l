%option noyywrap
%{
    #include <yacc/parser.tab.h>

    int yyLineNumber = 1;
    int yylex();
%}

/*543fgrsdffsd*/

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




[a-zA-Z_]+ { yylval.string = strdup(yytext); return(Ident); };
[0-9]+    { yylval.num = atoi(yytext); return(ValInt); };
.;
%%
