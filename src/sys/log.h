#ifndef _SYS_ERR_H_
#define _SYS_ERR_H_

#include <stdio.h>
#include <sys/cdefs.h>

#define LOG_DEFAULT_STREAM stderr

#define LOG_LEVEL_ERROR         3
#define LOG_LEVEL_WARNING       2
#define LOG_LEVEL_INFORMATION   1
#define LOG_LEVEL_DEBUG         0

#define LOG_LEVEL LOG_LEVEL_DEBUG

#define LOG_MAX_BACKTRACE_FRAMES 64

#define LOG_STRING_PANIC       "Critical"
#define LOG_STRING_FATAL       "Fatal"
#define LOG_STRING_ERROR       "Error"
#define LOG_STRING_WARNING     "Warning"
#define LOG_STRING_INFORMATION "Information"
#define LOG_STRING_DEBUG       "Debug"

/**
 * @brief Panic is used in cases where the process is in an unrecoverable state.
 *        This macro will print debug information to stderr and call abort() to
 *        performa a ungracefull exit. No clean up possible.
 */
#define PANIC(format, ...) __panicf(__FILE_NAME__, __LINE__, __func__, format"\n", ##__VA_ARGS__)

/**
 * @brief Panic is used in cases where the process is in an invalid or undefined state.
 *        This macro will print debug information to stderr and call exit() to
 *        initiate a gracefull exit, giving the process the opportunity to clean up.
 */
#define FATAL(format, ...) __fatalf(__FILE_NAME__, __LINE__, __func__, format"\n", ##__VA_ARGS__)

/*
Standard log macros. These will not terminate the application.
Can be turned off by setting LOG_LEVEL. All logs which have smaller log numbers
will not print.
*/
#define ERROR(format, ...) __LOG(LOG_STRING_ERROR, LOG_LEVEL_ERROR, format"\n", ##__VA_ARGS__)
#define WARN(format, ...) __LOG(LOG_STRING_WARNING, LOG_LEVEL_WARNING, format"\n", ##__VA_ARGS__)
#define INFO(format, ...) __LOG(LOG_STRING_INFORMATION, LOG_LEVEL_INFORMATION, format"\n", ##__VA_ARGS__)
#define DEBUG(format, ...) __LOG(LOG_STRING_DEBUG, LOG_LEVEL_DEBUG, format"\n", ##__VA_ARGS__)

#define __LOG(level, priority, format, ...) \
    do { \
        if (LOG_LEVEL <= priority) \
            __logf(level, __FILE_NAME__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while(0)

void __logf(
    const char* restrict level,
    const char* restrict file,
    const unsigned long line,
    const char* restrict func,
    const char* restrict format,
    ...);

void __panicf(
    const char* restrict file,
    const unsigned long line,
    const char* restrict func,
    const char* restrict format,
    ...);

void __fatalf(
    const char* restrict file,
    const unsigned long line,
    const char* restrict func,
    const char* restrict format,
    ...);

void log_init(void);

void log_register_stream(FILE* restrict stream);

#endif /* _SYS_ERR_H_ */
