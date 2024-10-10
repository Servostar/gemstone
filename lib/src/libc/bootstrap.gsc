// ----------------------------------------
// Bootstrap module for libgscstd-glibc
// used on GNU/Linux operating systems

include "os"

// main function is to be implemented by the application source
#[nomangle]
fun main()

// entrypoint function
#[nomangle,noreturn,entry]
fun _start() {
    main()
    std::os::exit(0 as i32)
}
