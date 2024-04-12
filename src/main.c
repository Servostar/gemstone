#include <stdlib.h>
#include <sys/log.h>
#include <yacc/parser.tab.h>

#define LOG_LEVEL LOG_LEVEL_DEBUG

void notify_exit(void)
{
    DEBUG("Exiting gemstone...");
}

void setup(void)
{
    log_init();
    DEBUG("starting gemstone...");

    #if LOG_LEVEL <= LOG_LEVEL_DEBUG
    atexit(&notify_exit);
    #endif

    DEBUG("finished starting up gemstone...");
}

int main() {
    setup();

    yyparse();
    return 0;
}