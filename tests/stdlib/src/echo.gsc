
import "std"

cstr: EOL = "\n"

fun main() {

    handle: stdin = nullHandle
    getStdinHandle(stdin)

    handle: stdout = nullHandle
    getStdoutHandle(stdout)

    ref u8: buffer = 0 as ref u8
    heapAlloc(256)(buffer)

    u32: bytesRead = 0 as u32
    readBytes(stdin, buffer, 8)(bytesRead)

    u32: bytesWritten = 0 as u32
    writeBytes(stdout, buffer, bytesRead)(bytesWritten)
    writeBytes(stdout, EOL, 1)(bytesWritten)

    heapFree(buffer)
}
