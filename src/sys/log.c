
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/log.h>
#include <assert.h>

static struct Logger_t {
    FILE** streams;
    size_t stream_count;
} GlobalLogger;

void log_init(void)
{
    assert(LOG_DEFAULT_STREAM != NULL);
    log_register_stream(LOG_DEFAULT_STREAM);
}

void log_register_stream(FILE* restrict stream)
{
    // replace runtime check with assertion
    // only to be used in debug target
    assert(stream != NULL);

    if (GlobalLogger.stream_count == 0)
    {
        GlobalLogger.streams = (FILE**) malloc(sizeof(FILE*));
        GlobalLogger.stream_count = 1;

        if (GlobalLogger.streams == NULL)
        {
            PANIC("failed to allocate stream buffer");
        }   
    }
    else
    {
        GlobalLogger.stream_count++;
        size_t bytes = GlobalLogger.stream_count * sizeof(FILE*);
        GlobalLogger.streams = (FILE**) realloc(GlobalLogger.streams, bytes);

        if (GlobalLogger.streams == NULL)
        {
            PANIC("failed to reallocate stream buffer");
        }
    }

    GlobalLogger.streams[GlobalLogger.stream_count - 1] = stream;
}

static void vflogf(
    FILE* restrict stream,
    const char* restrict level,
    const char* restrict file,
    const unsigned long line,
    const char* restrict func,
    const char* restrict format,
    va_list args)
{
    fprintf(stream, "%s in %s() at %s:%lu: ", level, func, file, line);
    vfprintf(stream, format, args);
}

void syslog_logf(
    const char* restrict level,
    const char* restrict file,
    const unsigned long line,
    const char* restrict func,
    const char* restrict format,
    ...)
{
    va_list args;
    va_start(args, format);

    for (size_t i = 0; i < GlobalLogger.stream_count; i++)
    {
        FILE* stream = GlobalLogger.streams[i];

        vflogf(stream, level, file, line, func, format, args);
    }

    va_end(args);
}

void syslog_panicf(
    const char* restrict file,
    const unsigned long line,
    const char* restrict func,
    const char* restrict format,
    ...)
{
    va_list args;
    va_start(args, format);

    vflogf(stderr, LOG_STRING_PANIC, file, line, func, format, args);

    va_end(args);

    exit(EXIT_FAILURE);
}

void syslog_fatalf(
    const char* restrict file,
    const unsigned long line,
    const char* restrict func,
    const char* restrict format,
    ...)
{
    va_list args;
    va_start(args, format);

    vflogf(stderr, LOG_STRING_FATAL, file, line, func, format, args);

    va_end(args);

    abort();
}
