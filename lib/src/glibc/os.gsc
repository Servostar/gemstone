
include "types"

# from unistd.h
fun _exit(in i32: code)

# Return control back to the operating system
fun exit(in i32: code) {
    _exit(code)
}
