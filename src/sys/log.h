#ifndef _SYS_ERR_H_
#define _SYS_ERR_H_

#include <stdio.h>

#define LOG_DEFAULT_STREAM stderr

#define LOG_LEVEL_ERROR       3
#define LOG_LEVEL_WARNING     2
#define LOG_LEVEL_INFORMATION 1
#define LOG_LEVEL_DEBUG       0

#define LOG_LEVEL LOG_LEVEL_DEBUG

#define LOG_STRING_PANIC       "Critical"
#define LOG_STRING_FATAL       "Fatal"
#define LOG_STRING_ERROR       "Error"
#define LOG_STRING_WARNING     "Warning"
#define LOG_STRING_INFORMATION "Information"
#define LOG_STRING_DEBUG       "Debug"

// define __FILE_NAME__ macro if not defined
// generally not defined by GCC < 11.3 and MSVC
#ifndef __FILE_NAME__
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#define __FILE_NAME__ \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILE_NAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#endif

/**
 * @brief Panic is used in cases where the process is in an unrecoverable state.
 *        This macro will print debug information to stderr and call abort() to
 *        performa a ungracefull exit. No clean up possible.
 */
#define PANIC(format, ...) \
    syslog_panicf(__FILE_NAME__, __LINE__, __func__, format "\n", ##__VA_ARGS__)

/**
 * @brief Panic is used in cases where the process is in an invalid or undefined
 * state. This macro will print debug information to stderr and call exit() to
 *        initiate a gracefull exit, giving the process the opportunity to clean
 * up.
 */
#define FATAL(format, ...) \
    syslog_fatalf(__FILE_NAME__, __LINE__, __func__, format "\n", ##__VA_ARGS__)

/*
Standard log macros. These will not terminate the application.
Can be turned off by setting LOG_LEVEL. All logs which have smaller log numbers
will not print.
*/
#define ERROR(format, ...) \
    __LOG(LOG_STRING_ERROR, LOG_LEVEL_ERROR, format "\n", ##__VA_ARGS__)
#define WARN(format, ...) \
    __LOG(LOG_STRING_WARNING, LOG_LEVEL_WARNING, format "\n", ##__VA_ARGS__)
#define INFO(format, ...)                                             \
    __LOG(LOG_STRING_INFORMATION, LOG_LEVEL_INFORMATION, format "\n", \
          ##__VA_ARGS__)
#define DEBUG(format, ...) \
    __LOG(LOG_STRING_DEBUG, LOG_LEVEL_DEBUG, format "\n", ##__VA_ARGS__)

extern int runtime_log_level;

#define __LOG(level, priority, format, ...)                                   \
    do {                                                                      \
        if (LOG_LEVEL <= priority)                                            \
            if (runtime_log_level <= priority)                                \
                syslog_logf(level, __FILE_NAME__, __LINE__, __func__, format, \
                            ##__VA_ARGS__);                                   \
    } while (0)

/**
 * @brief Set the runtime log level. Must be one of: LOG_LEVEL_ERROR,
 * LOG_LEVEL_WARNING, LOG_LEVEL_INFORMATION, LOG_LEVEL_DEBUG
 * @param level the new log level
 */
void set_log_level(int level);

/**
 * @brief Log a message into all registered streams
 *
 * @param level of the message
 * @param file origin of the message cause
 * @param line line in which log call was made
 * @param func function the log call was done in
 * @param format the format to print following args in
 * @param ...
 */
[[gnu::nonnull(1), gnu::nonnull(2), gnu::nonnull(4)]]
void syslog_logf(const char* restrict level, const char* restrict file,
                 unsigned long line, const char* restrict func,
                 const char* restrict format, ...);

/**
 * @brief Log a panic message to stderr and perform gracefull crash with exit()
 * denoting a failure
 *
 * @param file origin of the message cause
 * @param line line in which log call was made
 * @param func function the log call was done in
 * @param format the format to print following args in
 * @param ...
 */
[[noreturn]] [[gnu::nonnull(1), gnu::nonnull(3), gnu::nonnull(4)]]
void syslog_panicf(const char* restrict file, unsigned long line,
                   const char* restrict func, const char* restrict format, ...);

/**
 * @brief Log a critical message to stderr and perform ungracefull crash with
 * abort()
 *
 * @param file origin of the message cause
 * @param line line in which log call was made
 * @param func function the log call was done in
 * @param format the format to print following args in
 * @param ...
 */
[[noreturn]] [[gnu::nonnull(1), gnu::nonnull(3), gnu::nonnull(4)]]
void syslog_fatalf(const char* restrict file, unsigned long line,
                   const char* restrict func, const char* restrict format, ...);

/**
 * @brief Initialize the logger by registering stderr as stream
 *
 */
void log_init(void);

/**
 * @brief Register a stream as output source. Must be freed manually at exit if
 * necessary
 *
 * @param stream
 */
[[gnu::nonnull(1)]]
void log_register_stream(FILE* restrict stream);

#endif /* _SYS_ERR_H_ */
