//
// Created by servostar on 18.07.24.
//

#ifndef GEMSTONE_LIB_H
#define GEMSTONE_LIB_H

#include <link/driver.h>

typedef BinaryDriver* (*driver_init)();

void link_init();

bool link_run(TargetConfig*, TargetLinkConfig*);

void link_print_available_driver();

char* build_platform_library_name(char* basename, bool shared);

#endif // GEMSTONE_LIB_H
