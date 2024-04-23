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

/**
 * @brief Closes File after compiling.
 * 
 */

void close_file(void)
{
    fclose(argv[1]);
}

int main(int argc, char *argv[]) {

    setup();
    atexit(close_file);

    // Check for file input as argument
    if (2 != argc)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        PANIC("No File could be found");
    }
    
    // filename as first argument
    char *filename = argv[1];

    FILE *file = fopen(filename, "r");

    if (NULL == file)
    {
        PANIC("File couldn't be opened!");
    }

    yyparse();

    return 0;
}
