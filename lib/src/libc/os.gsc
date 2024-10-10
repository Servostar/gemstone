
include "types"

// from unistd.h
#[nomangle,preserve,noreturn]
fun _exit(in i32: code)

#[noreturn]
fun exit(in i32: code) {
    _exit(code)
}
