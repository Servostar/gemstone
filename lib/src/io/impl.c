
#include <io/api.h>
#include <capi.h>

#if defined(PLATFORM_WINDOWS)

// Compile for Windows

#include <Windows.h>

// FIXME: error in case GetStdHandle return INVALID_HANDLE_VALUE 
// FIXME: error in case functions return 0 

void getStdinHandle(handle* stdin) {
    *stdin = (handle) GetStdHandle(STD_INPUT_HANDLE);
}

void getStdoutHandle(handle* stdout) {
    *stdout = (handle) GetStdHandle(STD_OUTPUT_HANDLE);
}

void getStderrHandle(handle* stderr) {
    *stderr = (handle) GetStdHandle(STD_ERROR_HANDLE);
}

void writeBytes(handle dev, u8* buf, u32 len, u32* bytesWritten) {
    WriteFile((HANDLE) dev, buf, len, bytesRead, NULL);
}

void readBytes(handle dev, u8* buf, u32 len, u32* bytesRead) {
    ReadFile((HANDLE) dev, buf, len, bytesRead, NULL);
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

void getStdinHandle(handle* stdin) {
    *stdin = (handle) STDIN_FILENO;
}

void getStdoutHandle(handle* stdout) {
    *stdout = (handle) STDOUT_FILENO;
}

void getStderrHandle(handle* stderr) {
    *stderr = (handle) STDERR_FILENO;
}

void writeBytes(handle dev, u8* buf, u32 len, u32* bytesWritten) {
    *bytesWritten = write(TO_INT(dev), buf, len);
}

void readBytes(handle dev, u8* buf, u32 len, u32* bytesRead) {
    *bytesRead = read(TO_INT(dev), buf, len);
}

void flush(handle dev) {
    fsync(TO_INT(dev));
}


#endif
