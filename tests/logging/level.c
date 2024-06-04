//
// Created by servostar on 5/2/24.
//

#include "sys/log.h"
#include <sys/col.h>
#include <cfg/opt.h>

#define LOG_LEVEL LOG_LEVEL_WARNING

int main(int argc, char* argv[]) {
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
