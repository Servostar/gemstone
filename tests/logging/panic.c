//
// Created by servostar on 5/2/24.
//

#include "sys/log.h"
#include <cfg/opt.h>
#include <mem/cache.h>

int main(int argc, char* argv[]) {
    mem_init();
    parse_options(argc, argv);
    log_init();
    set_log_level(LOG_LEVEL_DEBUG);

    // this should appear in stderr
    INFO("before exit");

    PANIC("oooops something happened");

    // this should NOT appear in stderr
    //             ^^^
    ERROR("after exit");

    return 0;
}
