//
// Created by servostar on 5/31/24.
//

#ifndef GEMSTONE_OPT_H
#define GEMSTONE_OPT_H

#include <toml.h>

typedef struct TargetConfig_t {
    char* name;
    bool print_ast;
    bool print_asm;
    bool print_ir;
} TargetConfig;

typedef struct ProjectConfig_t {
    // name of the project
    char* name;
    // description
    char* desc;
    // version
    char* version;
    // list of authors
    char** authors;
} ProjectConfig;

TargetConfig default_target_config();

TargetConfig default_target_config_from_args(int argc, char* argv[]);

void print_help(void);

#endif //GEMSTONE_OPT_H
