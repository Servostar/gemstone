//
// Created by servostar on 5/31/24.
//

#include <cfg/opt.h>
#include <string.h>

#define MAX_TARGETS_PER_PROJECT 100

TargetConfig default_target_config() {
    TargetConfig config;

    config.name = "debug";
    config.print_ast = false;
    config.print_asm = false;
    config.print_ir = false;
    config.mode = Application;
    config.archive_directory = "archive";
    config.archive_directory = "bin";
    config.optimization_level = 1;
    config.root_module = NULL;

    return config;
}

TargetConfig default_target_config_from_args(int argc, char *argv[]) {
    TargetConfig config = default_target_config();

    for (int i = 0; i < argc; i++) {
        char *option = argv[i];

        if (strcmp(option, "--print-ast") == 0) {
            config.print_ast = true;
        } else if (strcmp(option, "--print-asm") == 0) {
            config.print_asm = true;
        } else if (strcmp(option, "--print-ir") == 0) {
            config.print_ir = true;
        } else if (strcmp(option, "--mode=app") == 0) {
            config.mode = Application;
        } else if (strcmp(option, "--mode=lib") == 0) {
            config.mode = Library;
        }
    }

    return config;
}

void print_help(void) {
    const char *lines[] = {
            "Gemstone Compiler (C) GPL-2.0\n",
            "Compile file(s): gsc <options> [files]",
            "Build project (build.toml): gsc [directory] [target]|all",
            "Options:",
            "    --print-ast      print resulting abstract syntax tree to a file",
            "    --print-asm      print resulting assembly language to a file",
            "    --print-ir       print resulting LLVM-IR to a file",
            "    --mode=[app|lib] set the compilation mode to either application or library"
    };

    for (unsigned int i = 0; i < sizeof(lines) / sizeof(const char *); i++) {
        printf("%s\n", lines[i]);
    }
}

#define PROJECT_OK 0
#define PROJECT_TOML_ERR 1
#define PROJECT_SEMANTIC_ERR 2

static void get_bool(bool* boolean, toml_table_t *table, const char* name) {
    toml_datum_t datum = toml_bool_in(table, name);

    if (datum.ok) {
        *boolean = datum.u.b;
    }
}

static void get_str(char** string, toml_table_t *table, const char* name) {
    toml_datum_t datum = toml_string_in(table, name);

    if (datum.ok) {
        *string = datum.u.s;
    }
}

static void get_int(int* integer, toml_table_t *table, const char* name) {
    toml_datum_t datum = toml_int_in(table, name);

    if (datum.ok) {
        *integer = (int) datum.u.i;
    }
}

static int parse_project_table(ProjectConfig *config, toml_table_t *project_table) {
    // project name
    get_str(&config->name, project_table, "version");
    if (config->name == NULL) {
        printf("Invalid project configuration: project must have a name\n\n");
        return PROJECT_SEMANTIC_ERR;
    }

    // project version
    get_str(&config->name, project_table, "version");

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
    TargetConfig target_config = default_target_config();

    target_config.name = (char*) name;

    get_bool(&target_config.print_ast, target_table, "print_ast");
    get_bool(&target_config.print_asm, target_table, "print_asm");
    get_bool(&target_config.print_ir, target_table, "print_ir");

    get_str(&target_config.root_module, target_table, "root");
    get_str(&target_config.output_directory, target_table, "output");
    get_str(&target_config.archive_directory, target_table, "archive");

    get_int(&target_config.optimization_level, target_table, "opt");

    char* mode = NULL;
    get_str(&mode, target_table, "mode");
    int err = get_mode_from_str(&target_config.mode, mode);
    if (err != PROJECT_OK) {
        return err;
    }

    g_array_append_val(config->targets, target_config);

    return PROJECT_OK;
}

static int parse_targets(ProjectConfig *config, toml_table_t *root) {
    toml_table_t *targets = toml_table_in(root, "target");

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
    FILE *config_file = fopen("build.toml", "r");
    if (config_file == NULL) {
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
            return parse_targets(config, project);
        }
    } else {
        printf("Invalid project configuration: missing project table\n\n");
    }

    toml_free(conf);
    return PROJECT_SEMANTIC_ERR;
}
