
import "std"

fun cstrlen(in cstr: str)(out u32: len) {
    u32: idx = 0 as u32

    while !(str[idx] == 0) {
        idx = idx + 1
    }

    len = idx
}

fun printcstr(in cstr: msg) {
    u32: len = 0
    cstrlen(msg)(len)

    handle: stdout = 0
    getStdoutHandle()(stdout)

    u32: written = 0
    writeBytes(stdout, msg, len)(written)
}

fun main() {
    printcstr("Hello, world!\n")
}
