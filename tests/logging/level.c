//
// Created by servostar on 5/2/24.
//

#include "sys/log.h"

#define LOG_LEVEL LOG_LEVEL_WARNING

int main(void) {
    log_init();

    DEBUG("logging some debug...");
    INFO("logging some info...");
    WARN("logging some warning...");
    ERROR("logging some error...");

    return 0;
}
