
#include <io/api.h>
#include <capi.h>

#if defined(PLATFORM_WINDOWS)

// Compile for Windows

#include <Windows.h>

// FIXME: error in case GetStdHandle return INVALID_HANDLE_VALUE 
// FIXME: error in case functions return 0 

handle getStdinHandle(handle* stdin) {
    return GetStdHandle(STD_INPUT_HANDLE);
}

handle getStdoutHandle(handle* stdout) {
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

handle getStderrHandle(handle* stderr) {
    return GetStdHandle(STD_ERROR_HANDLE);
}

u32 writeBytes(handle dev, u8* buf, u32 len) {
    u32 bytesWritten = 0;
    WriteFile((HANDLE) dev, buf, len, &bytesWritten, NULL);
    return bytesWritten;
}

u32 readBytes(handle dev, u8* buf, u32 len) {
    u32 bytesRead = 0;
    ReadFile((HANDLE) dev, buf, len, &bytesRead, NULL);
    return bytesRead;
}

void flush(handle dev) {
    FlushFileBuffers((HANDLE) dev);
}

#elif defined(PLATFORM_POSIX)

// Compile for Linux and BSD

#include <unistd.h>
#include <stdio.h>

// savely cast a 64-bit pointer down to a 32-bit value
// this assumes that 64-bit system will use 32-bit handles
// which are stored as 64-bit by zero extending
#define TO_INT(x) ((int)(long int)(x))

handle getStdinHandle() {
    return (handle) STDIN_FILENO;
}

handle getStdoutHandle() {
    return (handle) STDOUT_FILENO;
}

handle getStderrHandle() {
    return (handle) STDERR_FILENO;
}

u32 writeBytes(handle dev, u8* buf, u32 len) {
    return write(TO_INT(dev), buf, len);
}

u32 readBytes(handle dev, u8* buf, u32 len) {
    return read(TO_INT(dev), buf, len);
}

void flush(handle dev) {
    fsync(TO_INT(dev));
}


#endif
