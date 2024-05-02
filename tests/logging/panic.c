//
// Created by servostar on 5/2/24.
//

#include "sys/log.h"

int main(void) {
    log_init();

    // this should appear in stderr
    INFO("before exit");

    PANIC("oooops something happened");

    // this should NOT appear in stderr
    //             ^^^
    ERROR("after exit");

    return 0;
}
