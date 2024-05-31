//
// Created by servostar on 5/1/24.
//

#include "sys/log.h"
#include <sys/col.h>

int main(void) {
    log_init();
    col_init();

    DEBUG("logging some debug...");
    INFO("logging some info...");
    WARN("logging some warning...");
    ERROR("logging some error...");

    return 0;
}
