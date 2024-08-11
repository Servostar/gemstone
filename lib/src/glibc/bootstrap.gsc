# ----------------------------------------
# Bootstrap module for libgscstd-glibc
# used on GNU/Linux operating systems

import "os"

# main function is to be implemented by the application source
fun main()

# entrypoint function
fun start() {
    main()
    exit(0 as i32)
}
