//
// Created by servostar on 6/4/24.
//

#include <llvm/link/lld.h>
#include <sys/log.h>
#include <mem/cache.h>
#include <sys/col.h>

/*
 * call the LLD linker
 */
extern int lld_main(int Argc, const char **Argv, const char **outstr);

const char* get_absolute_link_path(const TargetConfig* config, const char* link_target_name) {

    for (guint i = 0; i < config->link_search_paths->len; i++) {
        const char* link_directory_path = g_array_index(config->link_search_paths, char*, i);

        char* path = g_build_filename(link_directory_path, link_target_name, NULL);
        char* cwd = g_get_current_dir();
        char* canonical = g_canonicalize_filename(path, cwd);

        const gboolean exists = g_file_test(canonical, G_FILE_TEST_EXISTS);
        const gboolean is_dir = g_file_test(canonical, G_FILE_TEST_IS_DIR);

        g_free(path);
        g_free(cwd);

        if (exists && !is_dir) {
            return canonical;
        }

        g_free(canonical);
    }

    // file not found
    return NULL;
}

TargetLinkConfig* lld_create_link_config(const Target* target, const TargetConfig* target_config, const Module* module) {
    DEBUG("generating link configuration");

    TargetLinkConfig* config = mem_alloc(MemoryNamespaceLld, sizeof(TargetLinkConfig));

    config->fatal_warnings = target_config->lld_fatal_warnings;
    config->object_file_names = g_array_new(FALSE, FALSE, sizeof(char*));
    config->colorize = stdout_supports_ansi_esc();

    // append build object file
    const char* target_object = get_absolute_link_path(target_config, (const char*) target->name.str);
    if (target_object == NULL) {
        ERROR("failed to resolve path to target object: %s", target->name.str);
        lld_delete_link_config(config);
        return NULL;
    }

    g_array_append_val(config->object_file_names, target_object);
    INFO("resolved path of target object: %s", target_object);

    // resolve absolute paths to dependent library object files
    DEBUG("resolving target dependencies...");
    for (guint i = 0; i < module->imports->len; i++) {
        const char* dependency = g_array_index(module->imports, const char*, i);

        const char* dependency_object = get_absolute_link_path(target_config, dependency);
        if (dependency_object == NULL) {
            ERROR("failed to resolve path to dependency object: %s", dependency);
            lld_delete_link_config(config);
            return NULL;
        }
        g_array_append_val(config->object_file_names, dependency_object);
        INFO("resolved path of target object: %s", dependency_object);
    }

    INFO("resolved %d dependencies", config->object_file_names->len);

    return config;
}

GArray* lld_create_lld_arguments(TargetLinkConfig* config) {
    GArray* argv = g_array_new(FALSE, FALSE, sizeof(char*));

    if (config->fatal_warnings) {
        g_array_append_val(argv, "--fatal-warnings");
    }

    if (config->colorize) {
        g_array_append_val(argv, "--color-diagnostics=always");
    }

    for (guint i = 0; i < config->object_file_names->len; i++) {
        char* object_file_path = g_array_index(config->object_file_names, char*, i);
        char* argument = g_strjoin("", "-l", object_file_path, NULL);
        g_array_append_val(argv, argument);
    }

    return argv;
}

BackendError lld_link_target(TargetLinkConfig* config) {
    BackendError err = SUCCESS;

    GArray* argv = lld_create_lld_arguments(config);

    const char* message = NULL;
    int status = lld_main((int) argv->len, (const char**) argv->data, &message);

    g_array_free(argv, TRUE);

    if (message != NULL) {
        print_message(Warning, "Message from LLD: %s", message);
        free((void*) message);
    }

    if (status != 0) {
        err = new_backend_impl_error(Implementation, NULL, "failed to link target");
    }

    return err;
}

void lld_delete_link_config(TargetLinkConfig* config) {
    for (guint i = 0; i < config->object_file_names->len; i++) {
        free((void*) g_array_index(config->object_file_names, const char*, i));
    }
    g_array_free(config->object_file_names, TRUE);
    mem_free(config);
}
