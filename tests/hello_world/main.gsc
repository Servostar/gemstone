
import "std"

fun u32:cstrlen(in cstr: str) {
    u32: idx = 0 as u32

    while !(str[idx] == 0) {
        idx = idx + 1 as u32
    }

    ret idx
}

fun printcstr(in cstr: msg) {
    u32: len = cstrlen(msg)

    handle: stdout = getStdoutHandle()

    writeBytes(stdout, msg, len)
}

fun int:main() {
    printcstr("Hello, world!\n")
    ret 0
}
