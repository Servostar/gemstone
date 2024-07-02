//
// Created by servostar on 6/3/24.
//

#ifndef GEMSTONE_STD_LIB_CAPI_H
#define GEMSTONE_STD_LIB_CAPI_H

#if defined(_WIN32) || defined (_WIN64)
#define PLATFORM_WINDOWS
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__linux__)
#define PLATFORM_POSIX
#endif

#endif // GEMSTONE_STD_LIB_CAPI_H
