#include <yacc/parser.tab.h>
#include <stdio.h>

extern FILE* yyin;

int main() {
    FILE* input = fopen("program.gem", "r");

    yyin = input;

    yyparse();
    return 0;
}