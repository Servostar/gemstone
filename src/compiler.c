//
// Created by servostar on 6/2/24.
//

#include <compiler.h>
#include <ast/ast.h>
#include <stdlib.h>
#include <sys/log.h>
#include <yacc/parser.tab.h>
#include <lex/util.h>
#include <io/files.h>
#include <assert.h>
#include <cfg/opt.h>
#include <codegen/backend.h>
#include <llvm/backend.h>
#include <mem/cache.h>
#include <set/set.h>
#include <link/lib.h>

#define GRAPHVIZ_FILE_EXTENSION "gv"

extern void yyrestart(FILE *);

// Module AST node used by the parser for AST construction.
[[maybe_unused]]
AST_NODE_PTR root;
// Current file which gets compiled the parser.
// NOTE: due to global state no concurrent compilation is possible
//       on parser level.
[[maybe_unused]]
ModuleFile *current_file;

/**
 * @brief Compile the specified file into AST
 * @param ast Initialized AST module node to build program rules
 * @param file The file to be processed
 * @return EXIT_SUCCESS in case the parsing was success full anything lese if not
 */
[[nodiscard("AST may be in invalid state")]]
[[gnu::nonnull(1), gnu::nonnull(1)]]
static int compile_file_to_ast(AST_NODE_PTR ast, ModuleFile *file) {
    assert(file->path != NULL);
    assert(ast != NULL);

    file->handle = fopen(file->path, "r");

    if (file->handle == NULL) {
        INFO("unable to open file: %s", file->path);
        print_message(Error, "Cannot open file %s: %s", file->path, strerror(errno));
        return EXIT_FAILURE;
    }

    DEBUG("parsing file: %s", file->path);
    // setup global state
    root = ast;
    current_file = file;
    yyin = file->handle;
    yyrestart(yyin);
    lex_reset();

    int status = yyparse();

    // clean up global state
    // current_file = NULL;
    root = NULL;
    yyin = NULL;

    return status;
}

/**
 * @brief Setup the environment of the target.
 * @param target
 * @return EXIT_SUCCESS if successful EXIT_FAILURE otherwise
 */
static int setup_target_environment(const TargetConfig *target) {
    DEBUG("setting up environment for target: %s", target->name);

    assert(target->output_directory != NULL);
    assert(target->archive_directory != NULL);

    int result = create_directory(target->archive_directory);
    if (result != 0 && errno != EEXIST) {
        const char *message = get_last_error();
        assert(message != NULL);

        print_message(Error, "Unable to create directory: %s: %s", target->archive_directory, message);
        free((void *) message);
        return EXIT_FAILURE;
    }

    result = create_directory(target->output_directory);
    if (result != 0 && errno != EEXIST) {
        const char *message = get_last_error();
        assert(message != NULL);

        print_message(Error, "Unable to create directory: %s: %s", target->output_directory, message);
        free((void *) message);
        return EXIT_FAILURE;
    }

    INFO("setup environment successfully");

    return EXIT_SUCCESS;
}

/**
 * @brief Print the supplied AST of the specified target to a graphviz ".gv" file
 * @param ast
 * @param target
 */
static void print_ast_to_file(AST_NODE_PTR ast, const TargetConfig *target) {
    assert(ast != NULL);
    assert(target != NULL);
    DEBUG("printing AST to file: %s", target->name);

    if (!target->print_ast) {
        INFO("no need to print AST");
        return;
    }

    // create file path to write graphviz to
    // basename of ile
    char* filename = g_strjoin(".", target->name, GRAPHVIZ_FILE_EXTENSION, NULL);
    // relative path to file
    char *path = g_build_filename(target->archive_directory, filename, NULL);

    DEBUG("Opening file to graph: %s", path);

    FILE *output = fopen(path, "w");
    if (output == NULL) {
        char *message = (char*) get_last_error();
        print_message(Error, "Unable to open file for syntax tree at: %s: %s", path, message);
        free(message);
    } else {
        DEBUG("writing graph to file...");

        AST_fprint_graphviz(output, ast);

        fclose(output);

        print_message(Info, "AST graph was written to: %s", path);
    }

    g_free(filename);
    g_free(path);
}

static void run_backend_codegen(const Module* module, const TargetConfig* target) {
    DEBUG("initializing LLVM codegen backend...");
    llvm_backend_init();

    DEBUG("initiializing backend for codegen...");
    BackendError err = init_backend();
    if (err.kind != Success) {
        return;
    }

    DEBUG("generating code...");
    err = generate_code(module, target);
    if (err.kind != Success) {
        print_message(Error, "Backend failed: %s", err.impl.message);
        return;
    }

    print_message(Info, "Compilation finished successfully");

    err = deinit_backend();
}

const char* get_absolute_import_path(const TargetConfig* config, const char* import_target_name) {
    INFO("resolving absolute path for import target: %s", import_target_name);

    if (!g_str_has_suffix(import_target_name, ".gsc")) {
        import_target_name = g_strjoin("", import_target_name, ".gsc", NULL);
    }

    for (guint i = 0; i < config->import_paths->len; i++) {
        const char* import_directory_path = g_array_index(config->import_paths, char*, i);

        char* path = g_build_filename(import_directory_path, import_target_name, NULL);
        char* cwd = g_get_current_dir();
        char* canonical = g_canonicalize_filename(path, cwd);

        const gboolean exists = g_file_test(canonical, G_FILE_TEST_EXISTS);
        const gboolean is_dir = g_file_test(canonical, G_FILE_TEST_IS_DIR);

        g_free(path);
        g_free(cwd);

        if (exists && !is_dir) {
            INFO("import target found at: %s", canonical);
            return canonical;
        }

        g_free(canonical);
    }

    // file not found
    return NULL;
}

static int compile_module_with_dependencies(ModuleFileStack *unit, ModuleFile* file, const TargetConfig *target, AST_NODE_PTR root_module) {

    GHashTable* imports = mem_new_g_hash_table(MemoryNamespaceAst, g_str_hash, g_str_equal);

    if (compile_file_to_ast(root_module, file) == EXIT_SUCCESS) {

        for (size_t i = 0; i < AST_get_child_count(root_module); i++) {
            AST_NODE_PTR child = AST_get_node(root_module, i);

            if (child->kind == AST_Import || child->kind == AST_Include) {

                const char* path = get_absolute_import_path(target, child->value);
                if (path == NULL) {
                    print_message(Error, "Cannot resolve path for import: `%s`", child->value);
                    return EXIT_FAILURE;
                }

                if (g_hash_table_contains(imports, path)) {
                    continue;
                }

                ModuleFile *imported_file = push_file(unit, path);
                AST_NODE_PTR imported_module = AST_new_node(empty_location(imported_file), AST_Module, NULL);

                if (compile_file_to_ast(imported_module, imported_file) == EXIT_SUCCESS) {
                    AST_merge_modules(root_module, i + 1, imported_module);
                } else {
                    return EXIT_FAILURE;
                }

                g_hash_table_insert(imports, (gpointer) path, NULL);

                gchar* directory = g_path_get_dirname(path);
                g_array_append_val(target->import_paths, directory);
            }
        }
    } else {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Build the given target
 * @param unit
 * @param target
 */
static void build_target(ModuleFileStack *unit, const TargetConfig *target) {
    print_message(Info, "Building target: %s", target->name);

    ModuleFile *file = push_file(unit, target->root_module);
    AST_NODE_PTR root_module = AST_new_node(empty_location(file), AST_Module, NULL);

    if (compile_module_with_dependencies(unit, file, target, root_module) == EXIT_SUCCESS) {
        if (root_module != NULL) {
            if (setup_target_environment(target) == 0) {

                print_ast_to_file(root_module, target);
                Module *module = create_set(root_module);

                if (module != NULL) {
                    run_backend_codegen(module, target);
                }
            }

            AST_delete_node(root_module);
        }
    }

    mem_purge_namespace(MemoryNamespaceLex);
    mem_purge_namespace(MemoryNamespaceAst);
    mem_purge_namespace(MemoryNamespaceSet);

    print_file_statistics(file);
}

/**
 * @brief Compile a single file.
 *        Creates a single target by the given command line arguments.
 * @param unit
 */
static void compile_file(ModuleFileStack *unit) {
    INFO("compiling basic files...");

    TargetConfig *target = default_target_config_from_args();

    if (target->root_module == NULL) {
        print_message(Error, "No input file specified.");
        delete_target_config(target);
        return;
    }

    build_target(unit, target);

    delete_target_config(target);
}

/**
 * @brief Build all project targets specified by the command line arguments.
 * @param unit
 * @param config
 */
static void build_project_targets(ModuleFileStack *unit, const ProjectConfig *config) {
    if (is_option_set("all")) {
        // build all targets in the project
        GHashTableIter iter;

        g_hash_table_iter_init(&iter, config->targets);

        char *key;
        TargetConfig *val;
        while (g_hash_table_iter_next(&iter, (gpointer) &key, (gpointer) &val)) {
            build_target(unit, val);
        }
        return;
    }

    // build all targets given in the arguments
    GArray* targets = get_non_options_after("build");

    if (targets != NULL) {
        for (guint i = 0; i < targets->len; i++) {
            const char *target_name = (((Option*) targets->data) + i)->string;

            if (g_hash_table_contains(config->targets, target_name)) {
                build_target(unit, g_hash_table_lookup(config->targets, target_name));
            } else {
                print_message(Error, "Unknown target: %s", target_name);
            }
        }

        g_array_free(targets, FALSE);
    } else {
        print_message(Error, "No targets specified.");
    }
}

/**
 * @brief Build targets from project. Configuration is provided by command line arguments.
 * @param unit File storage
 */
static void build_project(ModuleFileStack *unit) {
    ProjectConfig *config = default_project_config();
    int err = load_project_config(config);

    if (err == PROJECT_OK) {
        build_project_targets(unit, config);
    }

    delete_project_config(config);
}

void run_compiler() {
    ModuleFileStack files = new_file_stack();

    if (is_option_set("build")) {
        build_project(&files);
    } else if (is_option_set("compile")) {
        compile_file(&files);
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
