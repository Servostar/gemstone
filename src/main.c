#include <ast/ast.h>
#include <stdlib.h>
#include <sys/log.h>
#include <sys/col.h>
#include <lex/util.h>
#include <io/files.h>
#include <compiler.h>

/**
 * @brief Log a debug message to inform about beginning exit procedures
 *
 */
void notify_exit(void) { DEBUG("Exiting gemstone..."); }

/**
 * @brief Run compiler setup here
 *
 */
void setup(void) {
    // setup preample

    log_init();
    DEBUG("starting gemstone...");

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
    atexit(&notify_exit);
#endif

    // actual setup
    AST_init();

    col_init();

    lex_init();

    DEBUG("finished starting up gemstone...");
}

int main(int argc, char *argv[]) {
    setup();

    print_message(Info, "Running GSC version %s", GSC_VERSION);

    run_compiler(argc - 1, &argv[1]);

    return 0;
}
