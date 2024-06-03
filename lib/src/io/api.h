
#ifndef GEMSTONE_STD_LIB_IO_H_
#define GEMSTONE_STD_LIB_IO_H_

#include <def/api.h>

typedef ptr handle;

void getStdinHandle(handle* stdin);

void getStdoutHandle(handle* stdout);

void getStderrHandle(handle* stderr);

void writeBytes(handle dev, u8* buf, u32 len, u32* written);

void readBytes(handle dev, u8* buf, u32 len, u32* read);

void flush(handle dev);

#endif //GEMSTONE_STD_LIB_IO_H_
