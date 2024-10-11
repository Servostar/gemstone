# Gemstone Standard Library

## Modules

| Module name | Description                                                                      |
|-------------|----------------------------------------------------------------------------------|
| `std`       | Standard top level module importing all modules provided by the standard library |
| `types`     | Commonly used types and aliases in submodules                                    |
| `mem`       | Memory management for allocations, frees and copy operations                     |
| `os`        | Abstractions for common tasks related to the operating system                    |
| `io`        | Basic wrapper for general purpose synchronous input/output operations            |
| `fs`        | File system operations extending functionality of the `io` module                |
| `net`       | Module for networking capabilities                                               |
| `vec`       | Vector data type implemented as growable array                                   |
| `bootstrap` | Module required when compiling applications                                      |

## Bootstrap

This module contains sources required to boot a program, setup the runtime, libraries and eventually call the `main` function.
Since only applications make use of this boot code it is of no use when compiling a library.
It requires the `main` function to be defined. This function is the start of a gemstone applications and is called after setup with the following signature:
```
#[nomangle]
fun main()
```
In contrast to other programming languages such as `C` the main function does not exit with a return value.
In order to exit with a custom code use `std::os::exit()`.

## Operating System (OS)

## Memory management (mem)

This module compiles function for generic memory mangement closely integrated with the host operating system.
Included functionalities are: allocation, copy and clone operations.
This module is built around the idea that memory mangement happens at byte level.
Functions provided by this module are low-level functions for managing buffer of bytes without any considerations of higher abstractions of types.
Thus most functions interact with the folloing unsigned 8-bit integral type:
```
type unsigned half half: u8
```

### Invalid memory and null pointer

Gemstone has no pointers. References are conceptually no pointers but behave eqivalent on machine language level.
In constrast to other languages the is no null. However since it is convention that a memory adress of zero is to be considered invalid gemstone will treat any reference of value zero as invalid reference.
This property is used to check validity of references. It should be noted that this is not a fault prove method as the value of a reference can be changed at any point in time.
The following example will set a refernce to a reference to "point" to adress 97:
```
ref float: myValue = (97 as i32) to ref float
```

### Allocation

Buffers of bytes can be allocated by the operating system with the following `alloc` function:
```
fun ref u8:alloc(in u32: len)
```
This function allocates `len` bytes (as a buffers "length") and returns a reference to the buffer.
The buffer is guaranted to be a consecutive field of bytes all initialized to zero.
In case the allocations fails, for whatever reason, the returned reference will be zero (similar to `NULL` or `nullptr` in `C/C++`).
Implementationwise this function should use the standard platforms way of allocating memory from the global processes heap.

A block of memory allocated with `alloc` can be changed in length with `realloc`:
```
fun ref u8:realloc(in out ref u8: buf, in u32: len)
```
This function will reallocate an existing buffer (supplied by `buf`) to a new length and returns a reference to the buffer with the supplied length.
Behaviorwise, the supplied reference to a preexisting buffer will be overwritten to the newly allocated buffer.
The supplied reference and the returned result will be set to zero in case an error occurs.

Memory can be given back to the operating system with the free function:
```
fun free(in out ref u8: buf)
```
This function will give back the memory block specified by `buf`.
The reference supplied is the be considered invalid afterwards as it will be set to zero.

## Input and Output (IO)

## Networking (net)
