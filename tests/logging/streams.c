//
// Created by servostar on 5/2/24.
//

#include "sys/log.h"
#include <stdlib.h>
#include <cfg/opt.h>
#include <mem/cache.h>

static FILE* file;

void close_file(void) {
    if (file != NULL) {
        fclose(file);
    }
}

int main(int argc, char* argv[]) {
    mem_init();
    parse_options(argc, argv);
    log_init();
    set_log_level(LOG_LEVEL_DEBUG);

    // this should appear in stderr
    INFO("should only be in stderr");

    file = fopen("tmp/test.log", "w");
    if (file == NULL) {
        PANIC("could not open file");
    }
    atexit(close_file);

    log_register_stream(file);

    INFO("should be in both");

    return 0;
}
