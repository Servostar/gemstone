//
// Created by servostar on 5/31/24.
//

#include <cfg/opt.h>
#include <string.h>
#include <sys/log.h>

TargetConfig* default_target_config() {
    DEBUG("generating default target config...");

    TargetConfig* config = malloc(sizeof(TargetConfig));

    config->name = NULL;
    config->print_ast = false;
    config->print_asm = false;
    config->print_ir = false;
    config->mode = Application;
    config->archive_directory = NULL;
    config->archive_directory = NULL;
    config->optimization_level = 1;
    config->root_module = NULL;

    return config;
}

TargetConfig* default_target_config_from_args(int argc, char *argv[]) {
    DEBUG("generating default target from command line...");

    TargetConfig* config = default_target_config();

    for (int i = 0; i < argc; i++) {
        DEBUG("processing argument: %ld %s", i, argv[i]);
        char *option = argv[i];

        if (strcmp(option, "--print-ast") == 0) {
            config->print_ast = true;
        } else if (strcmp(option, "--print-asm") == 0) {
            config->print_asm = true;
        } else if (strcmp(option, "--print-ir") == 0) {
            config->print_ir = true;
        } else if (strcmp(option, "--mode=app") == 0) {
            config->mode = Application;
        } else if (strcmp(option, "--mode=lib") == 0) {
            config->mode = Library;
        } else {
            config->root_module = strdup(option);
        }
    }

    return config;
}

void print_help(void) {
    DEBUG("printing help dialog...");

    const char *lines[] = {
            "Gemstone Compiler (C) GPL-2.0",
            "Build a project target: gsc build [target]|all",
            "Compile non-project file: gsc compile <options> [file]",
            "Options:",
            "    --version        print the version",
            "    --print-ast      print resulting abstract syntax tree to a file",
            "    --print-asm      print resulting assembly language to a file",
            "    --print-ir       print resulting LLVM-IR to a file",
            "    --mode=[app|lib] set the compilation mode to either application or library"
    };

    for (unsigned int i = 0; i < sizeof(lines) / sizeof(const char *); i++) {
        printf("%s\n", lines[i]);
    }
}

static void get_bool(bool* boolean, toml_table_t *table, const char* name) {
    DEBUG("retrieving boolean %s", name);

    toml_datum_t datum = toml_bool_in(table, name);

    if (datum.ok) {
        *boolean = datum.u.b;
        DEBUG("boolean has value: %d", datum.u.b);
    }
}

static void get_str(char** string, toml_table_t *table, const char* name) {
    DEBUG("retrieving string %s", name);

    toml_datum_t datum = toml_string_in(table, name);

    if (datum.ok) {
        *string = datum.u.s;
        DEBUG("string has value: %s", datum.u.s);
    }
}

static void get_int(int* integer, toml_table_t *table, const char* name) {
    DEBUG("retrieving integer %s", name);

    toml_datum_t datum = toml_int_in(table, name);

    if (datum.ok) {
        *integer = (int) datum.u.i;
        DEBUG("integer has value: %ld", datum.u.i);
    }
}

static int parse_project_table(ProjectConfig *config, toml_table_t *project_table) {
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

static int parse_target(ProjectConfig *config, toml_table_t *target_table, const char* name) {
    DEBUG("parsing target table...");

    TargetConfig* target_config = default_target_config();

    target_config->name = (char*) name;

    get_bool(&target_config->print_ast, target_table, "print_ast");
    get_bool(&target_config->print_asm, target_table, "print_asm");
    get_bool(&target_config->print_ir, target_table, "print_ir");

    get_str(&target_config->root_module, target_table, "root");
    get_str(&target_config->output_directory, target_table, "output");
    get_str(&target_config->archive_directory, target_table, "archive");

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

static int parse_targets(ProjectConfig *config, toml_table_t *root) {
    DEBUG("parsing targets of project \"%s\"", config->name);

    toml_table_t *targets = toml_table_in(root, "target");
    if (targets == NULL) {
        printf("Project has no targets\n");
        return PROJECT_SEMANTIC_ERR;
    }

    config->targets = g_hash_table_new(g_str_hash, g_str_equal);

    for (int i = 0; i < MAX_TARGETS_PER_PROJECT; i++) {
        const char *key = toml_key_in(targets, i);

        if (!key)
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
        printf("Cannot open file %s: %s\n", PROJECT_CONFIG_FILE, strerror(errno));
        ERROR("project file not found");
        return PROJECT_TOML_ERR;
    }

    char err_buf[200];

    toml_table_t *conf = toml_parse_file(config_file, err_buf, sizeof(err_buf));
    fclose(config_file);

    if (!conf) {
        printf("Invalid project configuration: %s\n\n", err_buf);
        return PROJECT_SEMANTIC_ERR;
    }

    toml_table_t *project = toml_table_in(conf, "project");
    if (project) {
        if (parse_project_table(config, project) == PROJECT_OK) {
            return parse_targets(config, conf);
        }
    } else {
        printf("Invalid project configuration: missing project table\n\n");
    }

    toml_free(conf);
    return PROJECT_SEMANTIC_ERR;
}

void delete_target_config(TargetConfig* config) {
    if (config->root_module != NULL) {
        free(config->root_module);
    }
    if (config->archive_directory != NULL) {
        free(config->archive_directory);
    }
    if (config->name != NULL) {
        free(config->name);
    }
    if (config->output_directory != NULL) {
        free(config->output_directory);
    }
    free(config);
}

void delete_project_config(ProjectConfig* config) {
    if (config->name != NULL) {
        free(config->name);
    }
    if (config->authors != NULL) {
        g_array_free(config->authors, TRUE);
    }
    if (config->desc != NULL) {
        free(config->desc);
    }
    if (config->license != NULL) {
        free(config->license);
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

    free(config);
}

ProjectConfig* default_project_config() {
    ProjectConfig* config = malloc(sizeof(ProjectConfig));

    config->authors = NULL;
    config->name = NULL;
    config->targets = NULL;
    config->license = NULL;
    config->version = NULL;
    config->desc = NULL;

    return config;
}
