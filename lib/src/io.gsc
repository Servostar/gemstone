# Author: Sven Vogel
# Edited: 25.05.2024
# License: GPL-2.0

# ,----------------------------------------.
# |         Generic Input/Output           |
# `----------------------------------------`

include "def.gsc"

# platform specific handle to an I/O device
# can be a file, buffer, window or something else
# NOTE: this reference is not meant to be dereferenced
#       which can lead to errors and undefined behavior
type ptr: handle

# Returns a handle to this processes standard input I/O handle
# -- Implementation note
# On Linux this will return 0 as is it convention under UNIX (see: https://www.man7.org/linux/man-pages/man3/stdin.3.html)
# On Windows the library will call `GetStdHandle(STD_INPUT_HANDLE)`
fun getStdinHandle(out handle: stdin)

# Returns a handle to this processes standard input I/O handle
# -- Implementation note
# On Linux this will return 1 as is it convention under UNIX (see: https://www.man7.org/linux/man-pages/man3/stdout.3.html)
# On Windows the library will call `GetStdHandle(STD_OUTPUT_HANDLE)`
fun getStdoutHandle(out handle: stdout)

# Returns a handle to this processes standard input I/O handle
# -- Implementation note
# On Linux this will return 1 as is it convention under UNIX (see: https://www.man7.org/linux/man-pages/man3/stderr.3.html)
# On Windows the library will call `GetStdHandle(STD_OUTPUT_HANDLE)`
fun getStderrHandle(out handle: stderr)

# Write `len` number of bytes from `buf` into the I/O resource specified
# by `dev`. Returns the number of bytes written.
# -- Implementation note
# On Linux this will use the syscall write
# On Windows this will use the WriteFile function
fun writeBytes(in handle: dev, in ref u8: buf, in u32: len)(out u32: written)

# Read atmost `len` bytes to `buf` from the I/O resource specified by `dev`
# Returns the number of read bytes in `written`
# -- Implementation note
# On Linux this will use the syscall read
# On Windows this will use the ReadFile function
fun readBytes(in handle: dev, in ref u8: buf, in u32: len)(out u32: read)

# Flushes the buffers of the I/O resource specified by `dev`
# -- Implementation note
# On Linux this will use the fsync function
# On Windows this will use the FlushFileBuffers function
fun flush(in handle: dev)
