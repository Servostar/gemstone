#include <ast/ast.h>
#include <stdlib.h>
#include <sys/log.h>
#include <yacc/parser.tab.h>
#include <sys/col.h>
#include <lex/util.h>
#include <io/files.h>
#include <assert.h>
#include <cfg/opt.h>

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
        print_message(Error, "Cannot open file %s: %s", file->path, strerror(errno));
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

static void setup_target_environment(const TargetConfig* target) {
    if (target->output_directory) {
        
    }
}

void build_target(ModuleFileStack *unit, TargetConfig *target) {
    print_message(Info, "Compiling file: %s", target->root_module);

    TokenLocation location = new_location(0,0,0,0);
    AST_NODE_PTR ast = AST_new_node(location, AST_Module, NULL);
    ModuleFile *file = push_file(unit, target->root_module);

    if (compile_file_to_ast(ast, file) == EXIT_SUCCESS) {
        setup_target_environment(target);

        if (target->print_ast) {

        }

        // TODO: parse AST to semantic values
        // TODO: backend codegen
    }

    AST_delete_node(ast);

    print_file_statistics(file);
}

void compile_file(ModuleFileStack *unit, int argc, char *argv[]) {
    INFO("compiling basic files...");

    TargetConfig *target = default_target_config_from_args(argc, argv);

    if (target->root_module == NULL) {
        print_message(Error, "No input file specified.");
        delete_target_config(target);
        return;
    }

    build_target(unit, target);

    delete_target_config(target);
}

void build_project_targets(ModuleFileStack *unit, ProjectConfig *config, int argc, char *argv[]) {
    if (argc == 1 && strcmp(argv[0], "all") == 0) {
        GHashTableIter iter;

        g_hash_table_iter_init(&iter, config->targets);

        char *key;
        TargetConfig *val;
        while (g_hash_table_iter_next(&iter, (gpointer) &key, (gpointer) &val)) {
            build_target(unit, val);
        }
    } else {
        for (int i = 0; i < argc; i++) {
            char *target_name = argv[i];
            if (g_hash_table_contains(config->targets, target_name)) {
                build_target(unit, g_hash_table_lookup(config->targets, target_name));
            }
        }
    }
}

void build_project(ModuleFileStack *unit, int argc, char *argv[]) {
    if (argc <= 0) {
        print_message(Error, "No targets specified.");
        return;
    }

    ProjectConfig *config = default_project_config();
    int err = load_project_config(config);

    if (err == PROJECT_OK) {
        build_project_targets(unit, config, argc, argv);
    }

    delete_project_config(config);
}

void configure_run_mode(int argc, char *argv[]) {
    if (argc <= 0) {
        INFO("no arguments provided");
        print_help();
        return;
    }

    ModuleFileStack files;
    files.files = NULL;

    if (strcmp(argv[0], "build") == 0) {
        build_project(&files, argc - 1, &argv[1]);
    } else if (strcmp(argv[0], "compile") == 0) {
        compile_file(&files, argc - 1, &argv[1]);
    } else {
        print_message(Error, "Invalid mode of operation. Rerun with --help.");
    }

    if (files.files == NULL) {
        print_message(Error, "No input files, nothing to do.");
        exit(1);
    }

    print_unit_statistics(&files);
    delete_files(&files);
}

int main(int argc, char *argv[]) {

    setup();
    atexit(close_file);

    print_message(Info, "running GSC version %s", GSC_VERSION);

    configure_run_mode(argc - 1, &argv[1]);

    return 0;
}
