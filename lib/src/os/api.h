//
// Created by servostar on 6/3/24.
//

#ifndef GEMSTONE_STD_LIB_OS_H
#define GEMSTONE_STD_LIB_OS_H

#include <def/api.h>

void getPlatformName(cstr* name);

void getEnvVar(cstr name, cstr* value);

void setEnvVar(cstr name, cstr value);

void unsetEnvVar(cstr name);

#endif // GEMSTONE_STD_LIB_OS_H
