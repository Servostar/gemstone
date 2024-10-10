//
// Created by servostar on 8/11/24.
//

#include "lldc.h"

#include <io/files.h>
#include <mem/cache.h>
#include <stdio.h>

extern int lld_main(int Argc, const char **Argv, const char **outstr);

const char* FLAGS[] = {
    "--fatal-warnings"
};

const char* get_optimization_level_string(TargetConfig* config)
{
    char* buffer = mem_alloc(MemoryNamespaceLld, 6);

    sprintf(buffer, "-O%d", config->optimization_level);

    return buffer;
}

bool lldc_link(TargetConfig* target_config, TargetLinkConfig* link_config) {

    GArray* arguments = mem_new_g_array(MemoryNamespaceLld, sizeof(char*));

    char* linker = "ld.lld";
    g_array_append_val(arguments, linker);

    if (is_option_set("color-always")) {
        char* colored_diagnostics = "--color-diagnostics=always";
        g_array_append_val(arguments, colored_diagnostics);
    }

    const char* optimization_level = get_optimization_level_string(target_config);
    g_array_append_val(arguments, optimization_level);

    if (extract_sys_from_triple(target_config->triple) == SYS_LINUX)
    {
        if (target_has_shared_dependency(link_config))
        {
            // add dynamic linker
            print_message(Info, "Enabling dynamic linker for build");

            const char* enable_dynamic_linker = "--Bdynamic";
            g_array_append_val(arguments, enable_dynamic_linker);

            const char* default_dynamic_linker_path = "/usr/bin/ld.so";
            const char* dynamic_linker_option = "--dynamic-linker=";

            char* buffer = mem_alloc(MemoryNamespaceLld, strlen(dynamic_linker_option) + strlen(default_dynamic_linker_path) + 1);
            sprintf(buffer, "%s%s", dynamic_linker_option, default_dynamic_linker_path);
            g_array_append_val(arguments, buffer);
        }
    }

    for (int i = 0; i < sizeof(FLAGS)/sizeof(char*); i++) {
        char* flag = (char*) FLAGS[i];

        g_array_append_val(arguments, flag);
    }

    for (guint i = 0; i < link_config->object_file_names->len; i++) {
        char* obj = g_array_index(link_config->object_file_names, char*, i);
        g_array_append_val(arguments, obj);
    }

    char* output_flag = "-o";
    g_array_append_val(arguments, output_flag);
    g_array_append_val(arguments, link_config->output_file);

    size_t chars = 0;
    for (guint i = 0; i < arguments->len; i++) {
        chars += strlen(g_array_index(arguments, char*, i)) + 1;
    }

    char* buffer = mem_alloc(MemoryNamespaceLld, chars + 1);
    size_t offset = 0;
    for (guint i = 0; i < arguments->len; i++) {
        offset += sprintf(buffer + offset, "%s ", g_array_index(arguments, char*, i));
    }
    print_message(Info, buffer);

    const char* message = NULL;
    const bool code = lld_main(arguments->len, (const char**) arguments->data, &message);

    if (!code) {
        print_message(Error, message);
    }

    if (message != NULL) {
        free((void*) message);
    }

    return code;
}

BinaryDriver* lldc_get_driver() {

    BinaryDriver* driver = mem_alloc(MemoryNamespaceLld, sizeof(BinaryDriver));

    driver->name      = "ld.lld";
    driver->link_func = &lldc_link;

    return driver;
}

