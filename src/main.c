#include <stdlib.h>
#include <sys/log.h>
#include <yacc/parser.tab.h>

#define LOG_LEVEL LOG_LEVEL_DEBUG

/**
 * @brief Log a debug message to inform about beginning exit procedures
 * 
 */
void notify_exit(void)
{
    DEBUG("Exiting gemstone...");
}

/**
 * @brief Run compiler setup here
 * 
 */
void setup(void)
{
    // setup preample

    log_init();
    DEBUG("starting gemstone...");

    #if LOG_LEVEL <= LOG_LEVEL_DEBUG
    atexit(&notify_exit);
    #endif

    // actual setup

    DEBUG("finished starting up gemstone...");
}

int main() {
    setup();

    yyparse();
    return 0;
}