#include <ast/ast.h>
#include <stdlib.h>
#include <sys/log.h>
#include <sys/col.h>
#include <lex/util.h>
#include <cfg/opt.h>
#include <compiler.h>
#include <llvm/parser.h>
#include <mem/cache.h>
#include <link/lib.h>

/**
 * @brief Log a debug message to inform about beginning exit procedures
 *
 */
void notify_exit(void) { DEBUG("Exiting gemstone..."); }

/**
 * @brief Run compiler setup here
 *
 */
void setup(int argc, char *argv[]) {
    mem_init();

    // setup preample
    parse_options(argc, argv);

    log_init();
    DEBUG("starting gemstone...");

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
    atexit(&notify_exit);
#endif

    // actual setup
    AST_init();

    col_init();

    lex_init();

    link_init();

    DEBUG("finished starting up gemstone...");
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        print_help();
        exit(1);
    }

    setup(argc, argv);

    if (is_option_set("help")) {
        print_help();
        exit(0);
    }

    if (is_option_set("version")) {
        printf("Running GSC version %s\n", GSC_VERSION);
        exit(0);
    }

    if (is_option_set("list-targets")) {
        list_available_targets();
        exit(0);
    }

    if (is_option_set("list-driver")) {
        link_print_available_driver();
        exit(0);
    }

    run_compiler();

    if (is_option_set("print-gc-stats")) {
        print_memory_statistics();
    }

    return 0;
}
