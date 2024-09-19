//
// Created by servostar on 18.07.24.
//

#ifndef GEMSTONE_CLANG_DRIVER_H
#define GEMSTONE_CLANG_DRIVER_H

#include <link/driver.h>

bool clang_link(TargetConfig*, TargetLinkConfig* config);

BinaryDriver* clang_get_driver();

#endif // GEMSTONE_CLANG_DRIVER_H
