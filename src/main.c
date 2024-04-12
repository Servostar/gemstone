#include <sys/log.h>
#include <yacc/parser.tab.h>

#define LOG_LEVEL LOG_LEVEL_DEBUG

void setup(void)
{
    log_init();
    DEBUG("starting gemstone...");

    DEBUG("finished starting up gemstone...");
}

int main() {
    setup();

    yyparse();
    return 0;
}