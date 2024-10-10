//
// Created by servostar on 5/31/24.
//

#ifndef GEMSTONE_OPT_H
#define GEMSTONE_OPT_H

#include <glib.h>

#define MAX_TARGETS_PER_PROJECT 100
#define PROJECT_CONFIG_FILE     "build.toml"

#define PROJECT_OK           0
#define PROJECT_TOML_ERR     1
#define PROJECT_SEMANTIC_ERR 2

#define TOML_ERROR_MSG_BUF 256

typedef struct ModuleRef_t {
    GArray* module_path;
} ModuleRef;

guint module_ref_len(ModuleRef*);

char* module_ref_get(ModuleRef*, guint idx);

void module_ref_push(ModuleRef*, char*);

void module_ref_pop(ModuleRef*);

char* module_ref_to_str(ModuleRef*);

ModuleRef* module_ref_clone(ModuleRef*);

typedef struct TargetLinkConfig_t {
    // name of object files to link
    GArray* object_file_names;
    // treat warnings as errors
    gboolean fatal_warnings;
    // colorize linker output
    bool colorize;
    char* output_file;
    char* driver;
    // entry point symbol name
    char* entry;
} TargetLinkConfig;

typedef enum TargetCompilationMode_t {
    // output an executable binary
    Application,
    // output a binary object file
    Library
} TargetCompilationMode;

typedef enum DependencyKind_t { GemstoneProject, NativeLibrary } DependencyKind;

// Possible configuration modes:
// - build project (local/git)
// - native library (static/shared)
typedef struct Dependency_t {
    char* name;
    DependencyKind kind;
    union {
        struct {
            char* path;
            char* target;
        } project;
        struct {
            char* name;
            bool shared;
        } library;
    } mode;
    GArray* libraries;
} Dependency;

/**
 * @brief A target defines a source file which is to be compiled into a specific
 *        format. Additionally properties such as output folders can be set.
 *        Intermediate representations can be printed as well.
 */
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
    // output directory for intermediate representations (LLVM-IR, Assembly,
    // ...)
    char* archive_directory;
    // binary driver for executable generation
    char* driver;
    // system to compile code for
    // LLVM triple, see: https://clang.llvm.org/docs/CrossCompilation.html#target-triple
    // in case this is empty this will be the native platform
    char* triple;
    // mode of compilation
    TargetCompilationMode mode;
    // number between 1 and 3
    int optimization_level;
    // path to look for object files
    // (can be extra library paths, auto included is output_directory)
    GArray* link_search_paths;
    // treat linker warnings as errors
    bool lld_fatal_warnings;
    // treat parser warnings as errors
    bool gsc_fatal_warnings;
    GArray* import_paths;
    GHashTable* dependencies;
} TargetConfig;

/**
 * @brief A project is a collection of targets. Each target can point to
 *        very different sources. The project binds these targets together
 *        and specifies metadata about the authors and others.
 */
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

/**
 * @brief Represents a command line option.
 */
typedef struct Option_t {
    // index in which the option appeared in the argument array
    int index;
    // identifier of the option
    const char* string;
    // option if format is equals to --option=value
    const char* value;
    // whether or not this option has a value
    bool is_opt;
} Option;

extern const char* ARCH_X86_64;
extern const char* ARCH_I386;
extern const char* ARCH_ARM;
extern const char* ARCH_THUMB;
extern const char* ARCH_MIPS;

extern const char* SUB_V5;
extern const char* SUB_V6M;
extern const char* SUB_V7A;
extern const char* SUB_V7M;

extern const char* VENDOR_PC;
extern const char* VENDOR_APPLE;
extern const char* VENDOR_NVIDIA;
extern const char* VENDOR_IBM;

extern const char* SYS_NONE;
extern const char* SYS_LINUX;
extern const char* SYS_WIN32;
extern const char* SYS_DARWIN;
extern const char* SYS_CUDA;

extern const char* ENV_EABI;
extern const char* ENV_GNU;
extern const char* ENV_ANDROID;
extern const char* ENV_MACHO;
extern const char* ENV_ELF;

const char* extract_arch_from_triple(const char* triple);

const char* extract_sub_from_triple(const char* triple);

const char* extract_vendor_from_triple(const char* triple);

const char* extract_sys_from_triple(const char* triple);

const char* extract_env_from_triple(const char* triple);

bool target_has_shared_dependency(TargetLinkConfig*);

/**
 * @brief Create the default configuration for targets.
 * @return A pointer to a new target configuration.
 */
[[nodiscard("must be freed")]]

TargetConfig* default_target_config();

/**
 * @brief Create the default configuration for a project.
 *        Contains no targets.
 * @return A pointer to a new project configuration.
 */
[[nodiscard("must be freed")]]

ProjectConfig* default_project_config();

/**
 * @brief Create a new default target configuration an write command line
 *        option onto the default configuration.
 * @return A default config with user specified values.
 */
[[nodiscard("must be freed")]]

TargetConfig* default_target_config_from_args();

/**
 * @brief Load a project configuration from a TOML file with the name
 *        PROJECT_CONFIG_FILE.
 * @param config Configuration to fill values from file into.
 * @return
 */
[[gnu::nonnull(1)]]

int load_project_config(ProjectConfig* config);

/**
 * @brief Print a help dialog to stdout.
 */
void print_help(void);

/**
 * @brief Delete a project config by deallocation.
 * @param config The config to free.
 */
[[gnu::nonnull(1)]]

void delete_project_config(ProjectConfig* config);

/**
 * @brief Delete a target configuration by deallocation.
 */
[[gnu::nonnull(1)]]

void delete_target_config(TargetConfig*);

/**
 * @brief Parse the given command line arguments so that calls to
 *        is_option_set() and get_option() can be made safely.
 * @param argc Number of arguments
 * @param argv Array of arguments
 */
void parse_options(int argc, char* argv[]);

/**
 * @brief Tests whether an option was set as argument.
 * @attention Requires a previous call to parse_options()
 * @param option Name of option to check for.
 * @return 1 if the options was set, 0 otherwise
 */
[[gnu::nonnull(1)]]

bool is_option_set(const char* option);

/**
 * @brief Returns the options information if present
 * @attention Requires a previous call to parse_options()
 * @param option
 * @return A valid option struct or NULL if not found.
 */
[[gnu::nonnull(1)]]

const Option* get_option(const char* option);

/**
 * @brief Put a copy of all options whos index is greather than the index
 *        of the option specified by command.
 * @param command
 * @return an array of options that followed command.
 */
[[gnu::nonnull(1)]] [[nodiscard("must be freed")]]

GArray* get_non_options_after(const char* command);

void init_toml();

#endif // GEMSTONE_OPT_H
