//
// Created by servostar on 6/4/24.
//

#include <link/lib.h>
#include <llvm/link/lld.h>
#include <mem/cache.h>
#include <sys/col.h>
#include <sys/log.h>

const char* get_absolute_link_path(const TargetConfig* config,
                                   const char* link_target_name) {
    INFO("resolving absolute path for link target: %s", link_target_name);

    for (guint i = 0; i < config->link_search_paths->len; i++) {
        const char* link_directory_path =
          g_array_index(config->link_search_paths, char*, i);

        INFO("searching at: %s", link_directory_path);

        char* path =
          g_build_filename(link_directory_path, link_target_name, NULL);
        char* cwd              = g_get_current_dir();
        char* canonical        = g_canonicalize_filename(path, cwd);
        char* cached_canonical = mem_strdup(MemoryNamespaceLld, canonical);

        const gboolean exists = g_file_test(canonical, G_FILE_TEST_EXISTS);
        const gboolean is_dir = g_file_test(canonical, G_FILE_TEST_IS_DIR);

        g_free(path);
        g_free(cwd);
        g_free(canonical);

        if (exists && !is_dir) {
            INFO("link target found at: %s", cached_canonical);
            return cached_canonical;
        }
    }

    // file not found
    return NULL;
}

TargetLinkConfig* lld_create_link_config(__attribute__((unused))
                                         const Target* target,
                                         const TargetConfig* target_config,
                                         const Module* module) {
    DEBUG("generating link configuration");

    TargetLinkConfig* config =
      mem_alloc(MemoryNamespaceLld, sizeof(TargetLinkConfig));

    config->fatal_warnings = target_config->lld_fatal_warnings;
    config->object_file_names =
      mem_new_g_array(MemoryNamespaceLld, sizeof(char*));
    config->colorize = stdout_supports_ansi_esc();
    config->driver   = target_config->driver;

    // append build object file
    char* basename = g_strjoin(".", target_config->name, "o", NULL);
    char* filename =
      g_build_filename(target_config->archive_directory, basename, NULL);
    g_free(basename);
    const char* target_object =
      get_absolute_link_path(target_config, (const char*) filename);
    if (target_object == NULL) {
        ERROR("failed to resolve path to target object: %s", filename);
        g_free(filename);
        lld_delete_link_config(config);
        g_free(filename);
        return NULL;
    }
    g_free(filename);

    {
        // output file after linking
        basename = g_strjoin(".", target_config->name, "out", NULL);
        filename =
          g_build_filename(target_config->output_directory, basename, NULL);

        config->output_file = mem_strdup(MemoryNamespaceLld, filename);

        g_free(basename);
        g_free(filename);
    }

    g_array_append_val(config->object_file_names, target_object);
    INFO("resolved path of target object: %s", target_object);

    // resolve absolute paths to dependent library object files
    DEBUG("resolving target dependencies...");
    GHashTableIter iter;

    g_hash_table_iter_init(&iter, target_config->dependencies);
    char* key;
    Dependency* dep;
    while (g_hash_table_iter_next(&iter, (gpointer) &key, (gpointer) &dep)) {

        for (guint k = 0; k < dep->libraries->len; k++) {
            char* lib = g_array_index(dep->libraries, char*, k);
            if (lib == NULL)
                continue;

            // resolve path to object file
            if (g_file_test(lib, G_FILE_TEST_EXISTS)) {
                g_array_append_val(config->object_file_names, lib);
                continue;
            }

            char* path = get_absolute_link_path(target_config, lib);
            if (path == NULL) {
                print_message(Error, "Unable to resolve dependency: %s", lib);
            } else {
                g_array_append_val(config->object_file_names, path);
            }
        }
    }

    INFO("resolved %d dependencies", config->object_file_names->len);

    return config;
}

BackendError lld_link_target(TargetConfig* target_config, TargetLinkConfig* config) {

    if (link_run(target_config, config)) {
        return SUCCESS;
    }

    return new_backend_impl_error(Implementation, NULL, "linking failed");
}

void lld_delete_link_config(TargetLinkConfig* config) {
    mem_free(config->object_file_names);
    mem_free(config);
}
