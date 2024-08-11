//
// Created by servostar on 8/11/24.
//

#include "lldc.h"

#include <io/files.h>
#include <mem/cache.h>
#include <stdio.h>

extern int lld_main(int Argc, const char **Argv, const char **outstr);

bool lldc_link(TargetLinkConfig* config) {

    GArray* arguments = mem_new_g_array(MemoryNamespaceLld, sizeof(char*));

    char* linker = "ld.lld";
    g_array_append_val(arguments, linker);

    for (guint i = 0; i < config->object_file_names->len; i++) {
        char* obj = g_array_index(config->object_file_names, char*, i);
        g_array_append_val(arguments, obj);
    }
    char* output_flag = "-o";
    g_array_append_val(arguments, output_flag);
    g_array_append_val(arguments, config->output_file);

    for (guint i = 0; i < arguments->len; i++) {
        printf("%s ", g_array_index(arguments, char*, i));
    }
    printf("\n");

    const char* message = NULL;
    const bool code = lld_main(arguments->len, (const char**) arguments->data, &message);

    free((void*) message);

    if (!code) {
        print_message(Error, message);
    }

    return code;
}

BinaryDriver* lldc_get_driver() {

    BinaryDriver* driver = mem_alloc(MemoryNamespaceLld, sizeof(BinaryDriver));

    driver->name      = "ld.lld";
    driver->link_func = &lldc_link;

    return driver;
}

