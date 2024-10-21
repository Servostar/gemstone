
include "types"

type box: Handle {
    // UNIX file descriptor
    i32: fd
}

fun Handle: getStdoutHandle() {
    Handle: stdout

    stdout.fd = 1 as i32

    ret stdout
}

#[nomangle]
fun u64:write(in i32: fd, in ref u8: buf, in u64: count)

fun print(in ref u8: buf, in u32: n) {

    Handle: stdout = std::io::getStdoutHandle()

    write(1 as i32, buf, n as u64)
}
