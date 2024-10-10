//
// Created by servostar on 5/31/24.
//

#include <assert.h>
#include <cfg/opt.h>
#include <glib.h>
#include <io/files.h>
#include <link/driver.h>
#include <mem/cache.h>
#include <string.h>
#include <sys/log.h>
#include <toml.h>

static GHashTable* args = NULL;

static Dependency *new_dependency();

const char* ARCH_X86_64 = "x86_64";
const char* ARCH_I386 = "i386";
const char* ARCH_ARM = "arm";
const char* ARCH_THUMB = "thumb";
const char* ARCH_MIPS = "mips";

const char* SUB_V5 = "v5";
const char* SUB_V6M = "v6m";
const char* SUB_V7A = "v7a";
const char* SUB_V7M = "v7m";

const char* VENDOR_PC      = "pc";
const char* VENDOR_APPLE   = "apple";
const char* VENDOR_NVIDIA = "nvidia";
const char* VENDOR_IBM    = "ibm";

const char* SYS_NONE   = "none";
const char* SYS_LINUX  = "linux";
const char* SYS_WIN32  = "win32";
const char* SYS_DARWIN = "darwin";
const char* SYS_CUDA   = "cuda";

const char* ENV_EABI   = "eabi";
const char* ENV_GNU     = "gnu";
const char* ENV_ANDROID = "android";
const char* ENV_MACHO   = "macho";
const char* ENV_ELF    = "elf";

bool target_has_shared_dependency(TargetLinkConfig* config)
{
    bool has_shared = false;

    const char* shared_files[] = {
        ".so",
        ".dll"
    };

    for (guint i = 0; i < config->object_file_names->len; i++)
    {
        char* object_file = g_array_index(config->object_file_names, char*, i);

        for (int k = 0; k < sizeof(shared_files)/sizeof(char*); k++)
        {
            has_shared = g_str_has_suffix(object_file, shared_files[k]);
            if (has_shared)
                break;
        }

        if (has_shared)
            break;
    }

    return has_shared;
}

const char* find_string(const char* haystack, const char** options, size_t size)
{
    const static char* found = NULL;

    if (haystack != NULL)
    {
        for (size_t i = 0; i < size/sizeof(const char*); i++)
        {
            if (strstr(haystack, options[i]))
            {
                found = options[i];
                break;
            }
        }
    }

    return found;
}

const char* extract_arch_from_triple(const char* triple)
{
    const char* known_archs[] = {
        ARCH_X86_64,
        ARCH_I386,
        ARCH_ARM,
        ARCH_THUMB,
        ARCH_MIPS
    };

    return find_string(triple, known_archs, sizeof(known_archs));
}

const char* extract_sub_from_triple(const char* triple)
{
    const char* known_subs[] = {
        SUB_V5,
        SUB_V6M,
        SUB_V7A,
        SUB_V7M
    };

    return find_string(triple, known_subs, sizeof(known_subs));
}

const char* extract_vendor_from_triple(const char* triple)
{
    const char* known_subs[] = {
        VENDOR_PC,
        VENDOR_APPLE,
        VENDOR_NVIDIA,
        VENDOR_IBM
    };

    return find_string(triple, known_subs, sizeof(known_subs));
}

const char* extract_sys_from_triple(const char* triple)
{
    const char* known_sys[] = {
        SYS_NONE,
        SYS_LINUX,
        SYS_WIN32,
        SYS_DARWIN,
        SYS_CUDA
    };

    return find_string(triple, known_sys, sizeof(known_sys));
}

const char* extract_env_from_triple(const char* triple)
{
    const char* known_envs[] = {
        ENV_EABI,
        ENV_GNU,
        ENV_ANDROID,
        ENV_MACHO,
        ENV_ELF
    };

    return find_string(triple, known_envs, sizeof(known_envs));
}

guint module_ref_len(ModuleRef* ref) {
    return ref->module_path->len;
}

char* module_ref_get(ModuleRef* ref, guint idx) {
    return g_array_index(ref->module_path, char*, idx);
}

void module_ref_push(ModuleRef* ref, char* name) {
    char* chached_name = mem_strdup(MemoryNamespaceOpt, name);
    g_array_append_val(ref->module_path, chached_name);
}

void module_ref_pop(ModuleRef* ref) {
    if (ref->module_path->len > 0) {
        g_array_remove_index(ref->module_path, ref->module_path->len - 1);
    }
}

char* module_ref_to_str(ModuleRef* ref) {
    GString* string = g_string_new("");

    if (NULL != ref) {
        for (guint n = 0; n < ref->module_path->len; n++) {
            char* module = g_array_index(ref->module_path, char*, n);
            g_string_append(string, module);

            if (n + 1 < ref->module_path->len) {
                g_string_append_unichar(string, ':');
                g_string_append_unichar(string, ':');
            }
        }
    }

    char* cached_ref = mem_strdup(MemoryNamespaceAst, string->str);
    g_string_free(string, true);

    return cached_ref;
}

ModuleRef* module_ref_clone(ModuleRef* ref) {
    ModuleRef* module = mem_alloc(MemoryNamespaceOpt, sizeof(ModuleRef));

    module->module_path = mem_new_g_array(MemoryNamespaceOpt, sizeof(char*));

    for (guint n = 0; n < ref->module_path->len; n++) {
        char* cstr = g_array_index(ref->module_path, char*, n);
        char* copy = mem_strdup(MemoryNamespaceOpt, cstr);

        g_array_append_val(module->module_path, copy);
    }

    return module;
}

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
        option->string =
          mem_strdup(MemoryNamespaceOpt, argv[i] + (option->is_opt ? 2 : 0));
        option->index = i;
        option->value = NULL;

        char* equals = strchr(option->string, '=');
        if (equals != NULL) {
            option->value = equals + 1;
            *equals       = 0;
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

    GArray* array = mem_new_g_array(MemoryNamespaceOpt, sizeof(const char*));

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
        return NULL;
    }

    return array;
}

TargetConfig* default_target_config() {
    DEBUG("generating default target config...");

    TargetConfig* config = mem_alloc(MemoryNamespaceOpt, sizeof(TargetConfig));

    config->name               = mem_strdup(MemoryNamespaceOpt, "out");
    config->print_ast          = false;
    config->print_asm          = false;
    config->print_ir           = false;
    config->driver             = mem_strdup(MemoryNamespaceOpt, DEFAULT_DRIVER);
    config->mode               = Application;
    config->archive_directory  = mem_strdup(MemoryNamespaceOpt, "archive");
    config->output_directory   = mem_strdup(MemoryNamespaceOpt, "bin");
    config->optimization_level = 1;
    config->root_module        = NULL;
    config->link_search_paths =
      mem_new_g_array(MemoryNamespaceOpt, sizeof(char*));
    config->lld_fatal_warnings = FALSE;
    config->gsc_fatal_warnings = FALSE;
    config->import_paths = mem_new_g_array(MemoryNamespaceOpt, sizeof(char*));
    config->triple = NULL;

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
                print_message(Warning, "Invalid compilation mode: %s",
                              opt->value);
            }
        }
    }

    if (is_option_set("output")) {
        const Option* opt = get_option("output");

        if (opt->value != NULL) {
            config->name = mem_strdup(MemoryNamespaceOpt, (char*) opt->value);
        }
    }

    if (is_option_set("driver")) {
        const Option* opt = get_option("driver");

        if (opt->value != NULL) {
            config->driver = mem_strdup(MemoryNamespaceOpt, (char*) opt->value);
        }
    }

    char* cwd        = g_get_current_dir();
    char* cached_cwd = mem_strdup(MemoryNamespaceOpt, cwd);
    g_array_append_val(config->link_search_paths, cached_cwd);
    free(cwd);

    if (is_option_set("link-paths")) {
        const Option* opt = get_option("link-paths");

        if (opt->value != NULL) {

            const char* start = opt->value;
            const char* end   = NULL;
            while ((end = strchr(start, ',')) != NULL) {

                const int len   = end - start;
                char* link_path = mem_alloc(MemoryNamespaceOpt, len + 1);
                memcpy(link_path, start, len);
                link_path[len] = 0;

                g_array_append_val(config->link_search_paths, link_path);

                start = end;
            }

            const int len = strlen(start);
            if (len > 0) {
                char* link_path = mem_alloc(MemoryNamespaceOpt, len + 1);
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
            print_message(Warning, "Got more than one file to compile, using "
                                   "first, ignoring others.");
        }

        config->root_module =
          mem_strdup(MemoryNamespaceOpt, g_array_index(files, char*, 0));
    }

    char* default_import_path = mem_strdup(MemoryNamespaceOpt, ".");
    g_array_append_val(config->import_paths, default_import_path);

    if (is_option_set("import-paths")) {
        const Option* opt = get_option("import-paths");

        if (opt->value != NULL) {

            const char* start = opt->value;
            const char* end   = NULL;
            while ((end = strchr(start, ',')) != NULL) {

                const int len     = end - start;
                char* import_path = mem_alloc(MemoryNamespaceOpt, len + 1);
                memcpy(import_path, start, len);
                import_path[len] = 0;

                g_array_append_val(config->import_paths, import_path);

                start = end;
            }

            const int len = strlen(start);
            if (len > 0) {
                char* import_path = mem_alloc(MemoryNamespaceOpt, len + 1);
                memcpy(import_path, start, len);
                import_path[len] = 0;

                g_array_append_val(config->import_paths, import_path);
            }
        }
    }

    return config;
}

void print_help(void) {
    DEBUG("printing help dialog...");

    const char* lines[] = {
        "Gemstone Compiler (c) GPL-2.0",
        "Build a project target: gsc build [target]|all",
        "Compile non-project file: gsc compile <target-options> [file]",
        "Output information: gsc <option>",
        "Target options:",
        "    --print-ast           print resulting abstract syntax tree to a "
        "file",
        "    --print-asm           print resulting assembly language to a file",
        "    --print-ir            print resulting LLVM-IR to a file",
        "    --mode=[app|lib]      set the compilation mode to either "
        "application or library",
        "    --output=name         name of output files without extension",
        "    --driver              set binary driver to use",
        "    --link-paths=[paths,] set a list of directories to for libraries "
        "in",
        "    --all-fatal-warnings  treat all warnings as errors",
        "    --lld-fatal-warnings  treat linker warnings as errors",
        "    --gsc-fatal-warnings  treat parser warnings as errors",
        "Options:",
        "    --verbose        print logs with level information or higher",
        "    --debug          print debug logs (if not disabled at compile "
        "time)",
        "    --version        print the version",
        "    --list-targets   print a list of all available targets supported",
        "    --list-driver    print a list of all available binary driver",
        "    --help           print this help dialog",
        "    --color-always   always colorize output",
        "    --print-gc-stats print statistics of the garbage collector"};

    for (unsigned int i = 0; i < sizeof(lines) / sizeof(const char*); i++) {
        printf("%s\n", lines[i]);
    }
}

static bool get_bool(bool* boolean, const toml_table_t* table,
                     const char* name) {
    DEBUG("retrieving boolean %s", name);

    const toml_datum_t datum = toml_bool_in(table, name);

    if (datum.ok) {
        *boolean = datum.u.b;
        DEBUG("boolean has value: %d", datum.u.b);
    }

    return (bool) datum.ok;
}

static bool get_str(char** string, const toml_table_t* table,
                    const char* name) {
    DEBUG("retrieving string %s", name);

    const toml_datum_t datum = toml_string_in(table, name);

    if (datum.ok) {
        *string = datum.u.s;
        DEBUG("string has value: %s", datum.u.s);
    }

    return (bool) datum.ok;
}

static bool get_int(int* integer, const toml_table_t* table, const char* name) {
    DEBUG("retrieving integer %s", name);

    const toml_datum_t datum = toml_int_in(table, name);

    if (datum.ok) {
        *integer = (int) datum.u.i;
        DEBUG("integer has value: %ld", datum.u.i);
    }

    return (bool) datum.ok;
}

static void get_array(GArray* array, const toml_table_t* table,
                      const char* name) {
    const toml_array_t* toml_array = toml_array_in(table, name);

    if (toml_array) {
        for (int i = 0; i < toml_array_nelem(toml_array); i++) {
            toml_datum_t value = toml_string_at(toml_array, i);

            if (value.ok) {
                char* copy = mem_strdup(MemoryNamespaceOpt, value.u.s);
                g_array_append_val(array, copy);
            }
        }
    }
}

static int parse_project_table(ProjectConfig* config,
                               const toml_table_t* project_table) {
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
    toml_array_t* authors = toml_array_in(project_table, "authors");
    if (authors) {
        config->authors = mem_new_g_array(MemoryNamespaceOpt, sizeof(char*));

        for (int i = 0;; i++) {
            toml_datum_t author = toml_string_at(authors, i);
            if (!author.ok) {
                break;
            }

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
    print_message(Error, "Invalid project configuration, mode is invalid: %s",
                  name);
    return PROJECT_SEMANTIC_ERR;
}

static int parse_dependency(Dependency* dependency, toml_table_t* table, char* name) {
    dependency->name = mem_strdup(MemoryNamespaceOpt, name);

    bool is_project = false;
    is_project |= get_str(&dependency->mode.project.path, table, "build-path");
    is_project |= get_str(&dependency->mode.project.target, table, "target");

    bool is_library = false;
    is_library |= get_str(&dependency->mode.library.name, table, "library");
    is_library |= get_bool(&dependency->mode.library.shared, table, "shared");

    dependency->libraries = mem_new_g_array(MemoryNamespaceOpt, sizeof(char*));

    dependency->kind = is_project ? GemstoneProject : NativeLibrary;

    if (is_library && is_project) {
        print_message(Error, "Mutually exclusive configs found");
        return PROJECT_SEMANTIC_ERR;
    }

    if (!(is_library || is_project)) {
        print_message(Error, "Missing dependency config");
        return PROJECT_SEMANTIC_ERR;
    }

    return PROJECT_OK;
}

static int parse_target(const ProjectConfig* config,
                        const toml_table_t* target_table) {
    DEBUG("parsing target table...");

    TargetConfig* target_config = default_target_config();

    get_str(&target_config->name, target_table, "name");

    get_bool(&target_config->print_ast, target_table, "print_ast");
    get_bool(&target_config->print_asm, target_table, "print_asm");
    get_bool(&target_config->print_ir, target_table, "print_ir");

    get_str(&target_config->driver, target_table, "driver");
    get_str(&target_config->root_module, target_table, "root");
    get_str(&target_config->output_directory, target_table, "output");
    get_str(&target_config->archive_directory, target_table, "archive");
    get_bool(&target_config->lld_fatal_warnings, target_table,
             "lld_fatal_warnings");
    get_bool(&target_config->gsc_fatal_warnings, target_table,
             "gsc_fatal_warnings");

    get_int(&target_config->optimization_level, target_table, "opt");

    get_str(&target_config->triple, target_table, "triple");

    char* mode = NULL;
    get_str(&mode, target_table, "mode");
    int err = get_mode_from_str(&target_config->mode, mode);
    if (err != PROJECT_OK) {
        return err;
    }
    char* cwd        = g_get_current_dir();
    char* cached_cwd = mem_strdup(MemoryNamespaceOpt, cwd);
    free(cwd);

    g_array_append_val(target_config->link_search_paths, cached_cwd);
    get_array(target_config->link_search_paths, target_table, "link-paths");

    g_array_append_val(target_config->import_paths, cached_cwd);
    get_array(target_config->import_paths, target_table, "import-paths");

    g_hash_table_insert(config->targets, target_config->name, target_config);

    toml_table_t* dependencies = toml_table_in(target_table, "dependencies");
    if (dependencies) {
        target_config->dependencies = mem_new_g_hash_table(MemoryNamespaceOpt, g_str_hash, g_str_equal);
        for (int i = 0; i < toml_table_ntab(dependencies); i++) {
            char* key = (char*) toml_key_in(dependencies, i);

            if (key == NULL) {
                break;
            }

            toml_table_t* dependency_table = toml_table_in(dependencies, key);
            Dependency* dependency = new_dependency();
            if (parse_dependency(dependency, dependency_table, key) == PROJECT_SEMANTIC_ERR) {
                return PROJECT_SEMANTIC_ERR;
            }

            g_hash_table_insert(target_config->dependencies, mem_strdup(MemoryNamespaceOpt, key), dependency);
        }
    }

    return PROJECT_OK;
}

static Dependency* new_dependency() {
    Dependency* dependency = mem_alloc(MemoryNamespaceOpt, sizeof(Dependency));

    memset(dependency, 0, sizeof(Dependency));

    return dependency;
}

static int parse_targets(ProjectConfig* config, const toml_table_t* root) {
    DEBUG("parsing targets of project \"%s\"", config->name);

    toml_array_t* targets = toml_array_in(root, "targets");
    if (targets == NULL) {
        print_message(Warning, "Project has no targets");
        return PROJECT_SEMANTIC_ERR;
    }

    config->targets =
      mem_new_g_hash_table(MemoryNamespaceOpt, g_str_hash, g_str_equal);

    for (int i = 0; i < toml_array_nelem(targets); i++) {
        toml_table_t* target = toml_table_at(targets, i);
        parse_target(config, target);
    }

    return PROJECT_OK;
}

int load_project_config(ProjectConfig* config) {
    DEBUG("loading project config...");

    FILE* config_file = fopen(PROJECT_CONFIG_FILE, "r");
    if (config_file == NULL) {
        print_message(Error, "Cannot open file %s: %s", PROJECT_CONFIG_FILE,
                      strerror(errno));
        return PROJECT_SEMANTIC_ERR;
    }

    char err_buf[TOML_ERROR_MSG_BUF];

    toml_table_t* conf = toml_parse_file(config_file, err_buf, sizeof(err_buf));
    fclose(config_file);

    if (conf != NULL) {
        int status            = PROJECT_SEMANTIC_ERR;
        toml_table_t* project = toml_table_in(conf, "project");

        if (project != NULL) {
            if (parse_project_table(config, project) == PROJECT_OK) {
                status = parse_targets(config, conf);
            }
        } else {
            print_message(
              Error, "Invalid project configuration: missing project table.");
        }

        toml_free(conf);
        return status;
    } else {
        print_message(Error, "Invalid project configuration: %s", err_buf);
    }

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
            mem_free(g_array_index(config->link_search_paths, char*, i));
        }
    }
    mem_free(config);
}

void delete_project_config(ProjectConfig* config) {
    if (config->name != NULL) {
        mem_free(config->name);
    }
    if (config->authors != NULL) {
        mem_free(config->authors);
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
        while (
          g_hash_table_iter_next(&iter, (gpointer) &key, (gpointer) &val)) {
            delete_target_config(val);
        }

        mem_free(config->targets);
    }

    mem_free_from(MemoryNamespaceOpt, config);
}

ProjectConfig* default_project_config() {
    ProjectConfig* config =
      mem_alloc(MemoryNamespaceOpt, sizeof(ProjectConfig));

    config->authors = NULL;
    config->name    = NULL;
    config->targets = NULL;
    config->license = NULL;
    config->version = NULL;
    config->desc    = NULL;

    return config;
}

static void* toml_cached_malloc(size_t bytes) {
    return mem_alloc(MemoryNamespaceTOML, bytes);
}

static void toml_cached_free(void* ptr) {
    mem_free(ptr);
}

void init_toml() {
    INFO("setting up cached memory for TOML C99...");
    toml_set_memutil(toml_cached_malloc, toml_cached_free);
}
