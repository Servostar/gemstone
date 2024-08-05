# Author: Sven Vogel
# Edited: 25.05.2024
# License: GPL-2.0

# ,----------------------------------------.
# |            Memory Management           |
# `----------------------------------------`

include "def.gsc"

# Allocate `len` bytes of heap memory
# Returns a pointer to the memory as `ptr`
fun heapAlloc(in u32: len)(out ref u8: ptr)

# Rellocate `len` bytes of heap memory
# Returns a pointer to the memory as `ptr`
fun heapRealloc(in u32: len, in out ref u8: ptr)

# Free a block of memory
fun heapFree(in ref u8: ptr)

# Copy `len` bytes from `dst` into `src`
fun copy(in ref u8: dst, in ref u8: src, in u32: len)

# Fill `len` bytes of `dst` with `byte`
fun fill(in ref u8: dst, in u8: byte, in u32: len)

