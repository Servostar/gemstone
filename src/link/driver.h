//
// Created by servostar on 18.07.24.
//

#ifndef GEMSTONE_DRIVER_H
#define GEMSTONE_DRIVER_H

#include <cfg/opt.h>

#define DEFAULT_DRIVER "clang"

//! @brief Function a binary driver used to link files
typedef bool (*driver_link)(TargetLinkConfig*);

typedef struct BinaryDriver_t {
    const char* name;
    driver_link link_func;
} BinaryDriver;

#endif //GEMSTONE_DRIVER_H
