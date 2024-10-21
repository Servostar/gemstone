
include "types"

// ----------------------------------------------------------------
// external libc functions
// ................................................................

#[nomangle,preserve]
fun ref u8:malloc(in u32: size)

#[nomangle,preserve]
fun ref u8:realloc(in ref u8: buf, in u32: size)

#[nomangle,preserve]
fun free(in ref u8: buf)

// ----------------------------------------------------------------
// official specification implementation
// ................................................................

fun fill(in ref u8: buf, in u8: elem, in u32: len) {
    u32: idx = 0 as u32
    while idx < len {
        buf[idx] = elem
        idx = idx + 1 as u32
    }
}

fun ref u8:alloc(in u32: len) {
    ref u8: buf = malloc(len)
    std::mem::fill(buf, 0 as u8, len)
    ret buf
}

fun ref u8:realloc(in out ref u8: buf, in u32: len) {
    buf = realloc(buf, len)
    std::mem::fill(buf, 0 as u8, len)
    ret buf
}

fun free(in out ref u8: buf) {
    free(buf)
    buf = 0 to ref u8
}
