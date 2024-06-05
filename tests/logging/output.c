//
// Created by servostar on 5/1/24.
//

#include "sys/log.h"
#include <sys/col.h>
#include <cfg/opt.h>
#include <mem/cache.h>

int main(int argc, char* argv[]) {
    mem_init();
    parse_options(argc, argv);
    log_init();
    set_log_level(LOG_LEVEL_DEBUG);
    col_init();

    DEBUG("logging some debug...");
    INFO("logging some info...");
    WARN("logging some warning...");
    ERROR("logging some error...");

    return 0;
}
