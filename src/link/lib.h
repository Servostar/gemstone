//
// Created by servostar on 18.07.24.
//

#ifndef GEMSTONE_LIB_H
#define GEMSTONE_LIB_H

#include <link/clang/driver.h>

typedef BinaryDriver* (*driver_init)();

void link_init();

bool link_run(TargetLinkConfig*);

#endif //GEMSTONE_LIB_H
