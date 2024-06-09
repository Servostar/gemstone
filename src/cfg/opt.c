//
// Created by servostar on 5/31/24.
//

#include <cfg/opt.h>
#include <string.h>
#include <sys/log.h>
#include <io/files.h>
#include <assert.h>
#include <toml.h>
#include <mem/cache.h>

static GHashTable* args = NULL;

static void clean(void) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, args);

    while (g_hash_table_iter_next(&iter, &key, &value)) {
        mem_free(value);
        mem_free(key);
    }

    g_hash_table_destroy(args);
}

void parse_options(int argc, char* argv[]) {
    args = g_hash_table_new(g_str_hash, g_str_equal);

    atexit(clean);

    for (int i = 0; i < argc; i++) {
        Option* option = mem_alloc(MemoryNamespaceOpt, sizeof(Option));
        option->is_opt = g_str_has_prefix(argv[i], "--");
        option->string = mem_strdup(MemoryNamespaceOpt, argv[i] + (option->is_opt ? 2 : 0));
        option->index = i;
        option->value = NULL;

        char* equals = strchr(option->string, '=');
        if (equals != NULL) {
            option->value = equals + 1;
            *equals = 0;
        }

        g_hash_table_insert(args, (gpointer) option->string, (gpointer) option);
    }
}

bool is_option_set(const char* option) {
    assert(option != NULL);
    assert(args != NULL);
    return g_hash_table_contains(args, option);
}

const Option* get_option(const char* option) {
    if (g_hash_table_contains(args, option)) {
        return g_hash_table_lookup(args, option);
    }

    return NULL;
}

GArray* get_non_options_after(const char* command) {
    const Option* command_option = get_option(command);

    if (command_option == NULL) {
        return NULL;
    }

    GArray* array = g_array_new(FALSE, FALSE, sizeof(const char*));

    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, args);

    while (g_hash_table_iter_next(&iter, &key, &value)) {
        Option* option = (Option*) value;
        if (!option->is_opt && command_option->index < option->index) {
            g_array_append_val(array, option->string);
        }
    }

    if (array->len == 0) {
        g_array_free(array, FALSE);
        return NULL;
    }

    return array;
}

TargetConfig* default_target_config() {
    DEBUG("generating default target config...");

    TargetConfig* config = mem_alloc(MemoryNamespaceOpt, sizeof(TargetConfig));

    config->name = mem_strdup(MemoryNamespaceOpt, "out");
    config->print_ast = false;
    config->print_asm = false;
    config->print_ir = false;
    config->mode = Application;
    config->archive_directory = mem_strdup(MemoryNamespaceOpt, "archive");
    config->output_directory = mem_strdup(MemoryNamespaceOpt, "bin");
    config->optimization_level = 1;
    config->root_module = NULL;
    config->link_search_paths = g_array_new(FALSE, FALSE, sizeof(char*));
    config->lld_fatal_warnings = FALSE;
    config->gsc_fatal_warnings = FALSE;

    return config;
}

TargetConfig* default_target_config_from_args() {
    DEBUG("generating default target from command line...");

    TargetConfig* config = default_target_config();

    gboolean fatal_warnings = is_option_set("all-fatal-warnings");

    if (fatal_warnings || is_option_set("lld-fatal-warnings")) {
        config->lld_fatal_warnings = true;
    }

    if (fatal_warnings || is_option_set("gsc-fatal-warnings")) {
        config->gsc_fatal_warnings = true;
    }

    if (is_option_set("print-ast")) {
        config->print_ast = true;
    }

    if (is_option_set("print-asm")) {
        config->print_asm = true;
    }

    if (is_option_set("print-ir")) {
        config->print_ir = true;
    }

    if (is_option_set("mode")) {
        const Option* opt = get_option("mode");

        if (opt->value != NULL) {
            if (strcmp(opt->value, "app") == 0) {
                config->mode = Application;
            } else if (strcmp(opt->value, "lib") == 0) {
                config->mode = Library;
            } else {
                print_message(Warning, "Invalid compilation mode: %s", opt->value);
            }
        }
    }

    if (is_option_set("output")) {
        const Option* opt = get_option("output");

        if (opt->value != NULL) {
            config->name = mem_strdup(MemoryNamespaceOpt, (char*) opt->value);
        }
    }

    // TODO: free vvvvvvvvvvvvv
    char* cwd = g_get_current_dir();
    g_array_append_val(config->link_search_paths, cwd);

    if (is_option_set("link-paths")) {
        const Option* opt = get_option("link-paths");

        if (opt->value != NULL) {

            const char* start = opt->value;
            const char* end = NULL;
            while((end = strchr(start, ',')) != NULL) {

                const int len = end - start;
                char* link_path = malloc(len + 1);
                memcpy(link_path, start, len);
                link_path[len] = 0;

                g_array_append_val(config->link_search_paths, link_path);

                start = end;
            }

            const int len = strlen(start);
            if (len > 0) {
                char* link_path = malloc(len + 1);
                memcpy(link_path, start, len);
                link_path[len] = 0;

                g_array_append_val(config->link_search_paths, link_path);
            }
        }
    }

    GArray* files = get_non_options_after("compile");

    if (files == NULL) {
        print_message(Error, "No input file provided.");
    } else {

        if (files->len > 1) {
            print_message(Warning, "Got more than one file to compile, using first, ignoring others.");
        }

        config->root_module = mem_strdup(MemoryNamespaceOpt, ((char**) files->data) [0]);

        g_array_free(files, TRUE);
    }

    return config;
}

void print_help(void) {
    DEBUG("printing help dialog...");

    const char *lines[] = {
        "Gemstone Compiler (c) GPL-2.0",
        "Build a project target: gsc build [target]|all",
        "Compile non-project file: gsc compile <target-options> [file]",
        "Output information: gsc <option>",
        "Target options:",
        "    --print-ast           print resulting abstract syntax tree to a file",
        "    --print-asm           print resulting assembly language to a file",
        "    --print-ir            print resulting LLVM-IR to a file",
        "    --mode=[app|lib]      set the compilation mode to either application or library",
        "    --output=name         name of output files without extension",
        "    --link-paths=[paths,] set a list of directories to for libraries in",
        "    --all-fatal-warnings  treat all warnings as errors",
        "    --lld-fatal-warnings  treat linker warnings as errors",
        "    --gsc-fatal-warnings  treat parser warnings as errors",
        "Options:",
        "    --verbose        print logs with level information or higher",
        "    --debug          print debug logs (if not disabled at compile time)",
        "    --version        print the version",
        "    --list-targets   print a list of all available targets supported",
        "    --help           print this help dialog",
        "    --color-always   always colorize output",
        "    --print-gc-stats print statistics of the garbage collector"
    };

    for (unsigned int i = 0; i < sizeof(lines) / sizeof(const char *); i++) {
        printf("%s\n", lines[i]);
    }
}

static void get_bool(bool* boolean, const toml_table_t *table, const char* name) {
    DEBUG("retrieving boolean %s", name);

    const toml_datum_t datum = toml_bool_in(table, name);

    if (datum.ok) {
        *boolean = datum.u.b;
        DEBUG("boolean has value: %d", datum.u.b);
    }
}

static void get_str(char** string, const toml_table_t *table, const char* name) {
    DEBUG("retrieving string %s", name);

    const toml_datum_t datum = toml_string_in(table, name);

    if (datum.ok) {
        *string = datum.u.s;
        DEBUG("string has value: %s", datum.u.s);
    }
}

static void get_int(int* integer, const toml_table_t *table, const char* name) {
    DEBUG("retrieving integer %s", name);

    const toml_datum_t datum = toml_int_in(table, name);

    if (datum.ok) {
        *integer = (int) datum.u.i;
        DEBUG("integer has value: %ld", datum.u.i);
    }
}

static int parse_project_table(ProjectConfig *config, const toml_table_t *project_table) {
    DEBUG("parsing project table...");

    // project name
    get_str(&config->name, project_table, "version");
    if (config->name == NULL) {
        printf("Invalid project configuration: project must have a name\n\n");
        return PROJECT_SEMANTIC_ERR;
    }

    // project version
    get_str(&config->name, project_table, "name");

    // author names
    toml_array_t *authors = toml_array_in(project_table, "authors");
    if (authors) {
        config->authors = g_array_new(FALSE, FALSE, sizeof(char *));

        for (int i = 0;; i++) {
            toml_datum_t author = toml_string_at(authors, i);
            if (!author.ok)
                break;

            g_array_append_val(config->authors, author.u.s);
        }
    }

    // project description
    get_str(&config->desc, project_table, "description");
    // project license
    get_str(&config->license, project_table, "license");

    return PROJECT_OK;
}

static int get_mode_from_str(TargetCompilationMode* mode, const char* name) {
    if (mode == NULL) {
        return PROJECT_SEMANTIC_ERR;
    }

    if (strcmp(name, "application") == 0) {
        *mode = Application;
        return PROJECT_OK;
    } else if (strcmp(name, "library") == 0) {
        *mode = Library;
        return PROJECT_OK;
    }
    printf("Invalid project configuration, mode is invalid: %s\n\n", name);
    return PROJECT_SEMANTIC_ERR;
}

static int parse_target(const ProjectConfig *config, const toml_table_t *target_table, const char* name) {
    DEBUG("parsing target table...");

    TargetConfig* target_config = default_target_config();

    target_config->name = (char*) name;

    get_bool(&target_config->print_ast, target_table, "print_ast");
    get_bool(&target_config->print_asm, target_table, "print_asm");
    get_bool(&target_config->print_ir, target_table, "print_ir");

    get_str(&target_config->root_module, target_table, "root");
    get_str(&target_config->output_directory, target_table, "output");
    get_str(&target_config->archive_directory, target_table, "archive");
    get_bool(&target_config->lld_fatal_warnings, target_table, "lld_fatal_warnings");
    get_bool(&target_config->gsc_fatal_warnings, target_table, "gsc_fatal_warnings");

    get_int(&target_config->optimization_level, target_table, "opt");

    char* mode = NULL;
    get_str(&mode, target_table, "mode");
    int err = get_mode_from_str(&target_config->mode, mode);
    if (err != PROJECT_OK) {
        return err;
    }

    g_hash_table_insert(config->targets, target_config->name, target_config);

    return PROJECT_OK;
}

static int parse_targets(ProjectConfig *config, const toml_table_t *root) {
    DEBUG("parsing targets of project \"%s\"", config->name);

    toml_table_t *targets = toml_table_in(root, "target");
    if (targets == NULL) {
        print_message(Warning, "Project has no targets");
        return PROJECT_SEMANTIC_ERR;
    }

    config->targets = g_hash_table_new(g_str_hash, g_str_equal);

    for (int i = 0; i < MAX_TARGETS_PER_PROJECT; i++) {
        const char *key = toml_key_in(targets, i);

        if (key == NULL)
            break;

        toml_table_t *target = toml_table_in(targets, key);
        parse_target(config, target, key);
    }

    return PROJECT_OK;
}

int load_project_config(ProjectConfig *config) {
    DEBUG("loading project config...");

    FILE *config_file = fopen(PROJECT_CONFIG_FILE, "r");
    if (config_file == NULL) {
        print_message(Error, "Cannot open file %s: %s", PROJECT_CONFIG_FILE, strerror(errno));
        INFO("project file not found");
        return PROJECT_TOML_ERR;
    }

    char err_buf[TOML_ERROR_MSG_BUF];

    toml_table_t *conf = toml_parse_file(config_file, err_buf, sizeof(err_buf));
    fclose(config_file);

    if (conf == NULL) {
        print_message(Error, "Invalid project configuration: %s", err_buf);
        return PROJECT_SEMANTIC_ERR;
    }

    toml_table_t *project = toml_table_in(conf, "project");
    if (project == NULL) {
        print_message(Error, "Invalid project configuration: missing project table.");
    }

    if (parse_project_table(config, project) == PROJECT_OK) {
        return parse_targets(config, conf);
    }

    toml_free(conf);
    return PROJECT_SEMANTIC_ERR;
}

void delete_target_config(TargetConfig* config) {
    if (config->root_module != NULL) {
        mem_free(config->root_module);
    }
    if (config->archive_directory != NULL) {
        mem_free(config->archive_directory);
    }
    if (config->name != NULL) {
        mem_free(config->name);
    }
    if (config->output_directory != NULL) {
        mem_free(config->output_directory);
    }
    if (config->link_search_paths) {
        for (guint i = 0; i < config->link_search_paths->len; i++) {
            free(g_array_index(config->link_search_paths, char*, i));
        }
        g_array_free(config->link_search_paths, TRUE);
    }
    mem_free(config);
}

void delete_project_config(ProjectConfig* config) {
    if (config->name != NULL) {
        mem_free(config->name);
    }
    if (config->authors != NULL) {
        g_array_free(config->authors, TRUE);
    }
    if (config->desc != NULL) {
        mem_free(config->desc);
    }
    if (config->license != NULL) {
        mem_free(config->license);
    }
    if (config->targets != NULL) {
        GHashTableIter iter;

        g_hash_table_iter_init(&iter, config->targets);

        char* key;
        TargetConfig* val;
        while (g_hash_table_iter_next(&iter, (gpointer) &key, (gpointer) &val)) {
            delete_target_config(val);
        }

        g_hash_table_destroy(config->targets);
    }

    mem_free_from(MemoryNamespaceOpt, config);
}

ProjectConfig* default_project_config() {
    ProjectConfig* config = mem_alloc(MemoryNamespaceOpt, sizeof(ProjectConfig));

    config->authors = NULL;
    config->name = NULL;
    config->targets = NULL;
    config->license = NULL;
    config->version = NULL;
    config->desc = NULL;

    return config;
}
