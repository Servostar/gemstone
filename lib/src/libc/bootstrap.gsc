# ----------------------------------------
# Bootstrap module for libgscstd-glibc
# used on GNU/Linux operating systems

include "os"

# main function is to be implemented by the application source
fun main()

# entrypoint function
fun _start() {
    main()
    _exit(0 as i32)
}
