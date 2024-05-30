#include <ast/ast.h>
#include <stdlib.h>
#include <sys/log.h>
#include <yacc/parser.tab.h>
#include <sys/col.h>
#include <lex/util.h>
#include <io/files.h>
#include <assert.h>

extern void yyrestart(FILE *);

[[maybe_unused]]
AST_NODE_PTR root;
[[maybe_unused]]
ModuleFile *current_file;

/**
 * @brief Compile the specified file into AST
 * @param ast
 * @param file
 * @return EXIT_SUCCESS in case the parsing was success full anything lese if not
 */
[[nodiscard("AST may be in invalid state")]]
[[gnu::nonnull(1), gnu::nonnull(1)]]
static size_t compile_file_to_ast(AST_NODE_PTR ast, ModuleFile *file) {
    assert(file->path != NULL);
    assert(ast != NULL);

    file->handle = fopen(file->path, "r");

    if (file->handle == NULL) {
        INFO("unable to open file: %s", file->path);
        return 1;
    }

    DEBUG("parsing file: %s", file->path);
    // setup global state
    root = ast;
    current_file = file;
    yyin = file->handle;
    yyrestart(yyin);
    lex_reset();

    yyparse();

    // clean up global state
    // current_file = NULL;
    root = NULL;
    yyin = NULL;

    return 0;
}

/**
 * @brief Log a debug message to inform about beginning exit procedures
 *
 */
void notify_exit(void) { DEBUG("Exiting gemstone..."); }

/**
 * @brief Closes File after compiling.
 *
 */

void close_file(void) {
    if (NULL != yyin) {
        fclose(yyin);
    }
}

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
    atexit(close_file);

    ModuleFileStack files;
    files.files = NULL;

    for (int i = 1; i < argc; i++) {
        printf("Compiling file: %s\n\n", argv[i]);

        TokenLocation location = {
                .line_start = 0,
                .line_end = 0,
                .col_start = 0,
                .col_end = 0
        };
        AST_NODE_PTR ast = AST_new_node(location, AST_Module, NULL);
        ModuleFile *file = push_file(&files, argv[i]);

        if (compile_file_to_ast(ast, file) == EXIT_SUCCESS) {
            // TODO: parse AST to semantic values
            // TODO: backend codegen
        }

        AST_delete_node(ast);

        print_file_statistics(file);
    }

    print_unit_statistics(&files);

    delete_files(&files);
    return 0;
}
