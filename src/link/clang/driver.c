//
// Created by servostar on 18.07.24.
//

#include <io/files.h>
#include <link/clang/driver.h>
#include <mem/cache.h>

bool clang_link(TargetLinkConfig* config) {

    GString* commandString = g_string_new("");

    g_string_append(commandString, "clang");

    for (guint i = 0; i < config->object_file_names->len; i++) {
        g_string_append(commandString, " ");
        g_string_append(commandString,
                        g_array_index(config->object_file_names, char*, i));
    }

    g_string_append(commandString, " -o ");
    g_string_append(commandString, config->output_file);

    print_message(Info, "invoking binary link with: %s", commandString->str);

    if (system(commandString->str)) {
        return false;
    }

    g_string_free(commandString, true);

    return true;
}

BinaryDriver* clang_get_driver() {

    BinaryDriver* driver = mem_alloc(MemoryNamespaceLld, sizeof(BinaryDriver));

    driver->name      = "clang";
    driver->link_func = &clang_link;

    return driver;
}
