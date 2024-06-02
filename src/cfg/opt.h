//
// Created by servostar on 5/31/24.
//

#ifndef GEMSTONE_OPT_H
#define GEMSTONE_OPT_H

#include <toml.h>
#include <glib.h>

#define MAX_TARGETS_PER_PROJECT 100
#define PROJECT_CONFIG_FILE "build.toml"

#define PROJECT_OK 0
#define PROJECT_TOML_ERR 1
#define PROJECT_SEMANTIC_ERR 2

typedef enum TargetCompilationMode_t {
    Application,
    Library
} TargetCompilationMode;

typedef struct TargetConfig_t {
    char* name;
    bool print_ast;
    bool print_asm;
    bool print_ir;
    // root module file which imports all submodules
    // if this is NULL use the first commandline argument as root module
    char* root_module;
    // output directory for binaries
    char* output_directory;
    // output directory for intermediate representations (LLVM-IR, Assembly, ...)
    char* archive_directory;
    // mode of compilation
    TargetCompilationMode mode;
    // number between 1 and 3
    int optimization_level;
} TargetConfig;

typedef struct ProjectConfig_t {
    // name of the project
    char* name;
    // description
    char* desc;
    // version
    char* version;
    // license
    char* license;
    // list of authors
    GArray* authors;
    GHashTable* targets;
} ProjectConfig;

typedef struct Option_t {
    int index;
    const char* string;
    const char* value;
    bool is_opt;
} Option;

TargetConfig* default_target_config();

ProjectConfig* default_project_config();

TargetConfig* default_target_config_from_args();

int load_project_config(ProjectConfig *config);

void print_help(void);

void delete_project_config(ProjectConfig* config);

void delete_target_config(TargetConfig*);

void parse_options(int argc, char* argv[]);

[[gnu::nonnull(1)]]
bool is_option_set(const char* option);

[[gnu::nonnull(1)]]
const Option* get_option(const char* option);

[[gnu::nonnull(1)]]
[[nodiscard("must be freed")]]
GArray* get_non_options_after(const char* command);

#endif //GEMSTONE_OPT_H
