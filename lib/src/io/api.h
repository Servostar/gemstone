
#ifndef GEMSTONE_STD_LIB_IO_H_
#define GEMSTONE_STD_LIB_IO_H_

#include <def/api.h>

typedef ptr handle;

handle getStdinHandle();

handle getStdoutHandle();

handle getStderrHandle();

u32 writeBytes(handle dev, u8* buf, u32 len);

u32 readBytes(handle dev, u8* buf, u32 len);

void flush(handle dev);

#endif //GEMSTONE_STD_LIB_IO_H_
