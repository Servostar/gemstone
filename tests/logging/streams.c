//
// Created by servostar on 5/2/24.
//

#include "sys/log.h"
#include <stdlib.h>

static FILE* file;

void close_file(void) {
    if (file != NULL) {
        fclose(file);
    }
}

int main(void) {
    log_init();

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
