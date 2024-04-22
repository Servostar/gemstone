#include <stdlib.h>
#include <sys/log.h>
#include <yacc/parser.tab.h>

#define LOG_LEVEL LOG_LEVEL_DEBUG

extern FILE* yyin;

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

/**
 * @brief Closes File after compiling.
 * 
 */

void close_file(void)
{
    fclose(input);
}

int main(void) {
    setup();
    atexit(close_file);
    
    FILE* input = fopen("program.gem", "r");

    if (NULL == input)
    {
        ERROR("File couldn't be opened!");
    }
    
    yyin = input;

    yyparse();

    return 0;
}
