//
// Created by servostar on 18.07.24.
//

#ifndef GEMSTONE_GCC_DRIVER_H
#define GEMSTONE_GCC_DRIVER_H

#include <link/driver.h>

bool gcc_link(TargetConfig*, TargetLinkConfig* config);

BinaryDriver* gcc_get_driver();

#endif // GEMSTONE_GCC_DRIVER_H
