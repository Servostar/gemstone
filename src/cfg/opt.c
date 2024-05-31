//
// Created by servostar on 5/31/24.
//

#include <cfg/opt.h>
#include <string.h>

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

static int parse_project_table(ProjectConfig *config, toml_table_t *project_table) {
    // project name
    toml_datum_t name = toml_string_in(project_table, "name");
    if (!name.ok) {
        printf("Invalid project configuration: project must have a name\n\n");
        return PROJECT_SEMANTIC_ERR;
    }

    config->name = name.u.s;

    // project version
    toml_datum_t version = toml_string_in(project_table, "version");
    if (version.ok) {
        config->name = version.u.s;
    }

    // author names
    toml_array_t *authors = toml_array_in(project_table, "authors");
    if (authors) {
        config->authors = g_array_new(FALSE, FALSE, sizeof(char*));

        for (int i = 0;; i++) {
            toml_datum_t author = toml_string_at(authors, i);
            if (!author.ok)
                break;

            g_array_append_val(config->authors, author.u.s);
        }
    }

    // project description
    toml_datum_t description = toml_string_in(project_table, "description");
    if (description.ok) {
        config->desc = description.u.s;
    }

    // project license
    toml_datum_t license = toml_string_in(project_table, "license");
    if (license.ok) {
        config->license = license.u.s;
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
        parse_project_table(config, project);

        return PROJECT_OK;
    }
    printf("Invalid project configuration: missing project table\n\n");

    toml_free(conf);
    return PROJECT_SEMANTIC_ERR;
}
