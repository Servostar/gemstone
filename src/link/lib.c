//
// Created by servostar on 18.07.24.
//

#include <link/lib.h>
#include <mem/cache.h>
#include <sys/log.h>
#include <io/files.h>

static driver_init AVAILABLE_DRIVER[] = {
    clang_get_driver
};

static GHashTable* binary_driver = NULL;

void link_init() {
    INFO("initializing binary driver...");

    if (binary_driver == NULL) {
        binary_driver = mem_new_g_hash_table(MemoryNamespaceLld, g_str_hash, g_str_equal);

        for (unsigned long int i = 0; i < sizeof(AVAILABLE_DRIVER)/sizeof(driver_init); i++) {
            BinaryDriver* driver = AVAILABLE_DRIVER[i]();
            g_hash_table_insert(binary_driver, (gpointer) driver->name, driver);
            INFO("initialized `%s` driver", driver->name);
        }
    }
}

bool link_run(TargetLinkConfig* config) {

    if (g_hash_table_contains(binary_driver, config->driver)) {
        print_message(Info, "Invoking binary driver: %s", config->driver);

        BinaryDriver* driver = g_hash_table_lookup(binary_driver, config->driver);

        if (!driver->link_func(config))  {
            print_message(Error, "Driver %s failed", config->driver);
            return false;
        }
        return true;

    } else {
        print_message(Error, "Binary driver not available: `%s`", config->driver);
        return false;
    }
}
