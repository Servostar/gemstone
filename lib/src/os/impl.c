//
// Created by servostar on 6/3/24.
//

#include <capi.h>
#include <os/api.h>

#if defined(PLATFORM_WINDOWS)

void getPlatformName(cstr* name) {
    *name = (u8*) "windows";
}

#elif defined(PLATFORM_POSIX)

void getPlatformName(cstr * name) {
    *name = (u8*) "posix";
}

#endif

// Implementation based on libc

#include <stdlib.h>

void getEnvVar(cstr name, cstr* value) {
    *value = (cstr) getenv((char*) name);
}

void setEnvVar(cstr name, cstr value) {
    setenv((char*) name, (char*) value, true);
}

void unsetEnvVar(cstr name) {
    unsetenv((char*) name);
}
