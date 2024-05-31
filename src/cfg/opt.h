//
// Created by servostar on 5/31/24.
//

#ifndef GEMSTONE_OPT_H
#define GEMSTONE_OPT_H

#include <toml.h>
#include <glib.h>

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
    GArray* targets;
} ProjectConfig;

TargetConfig default_target_config();

TargetConfig default_target_config_from_args(int argc, char* argv[]);

void print_help(void);

#endif //GEMSTONE_OPT_H
