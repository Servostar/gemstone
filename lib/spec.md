# Gemstone Standard Library

## Modules

| Module name | Description                                                                      |
|-------------|----------------------------------------------------------------------------------|
| `std`       | Standard top level module importing all modules provided by the standard library |
| `types`     | Commonly used types and aliases in submodules                                    |
| `mem`       | Memory management for allocations, frees and copy operations                     |
| `os`        | Abstractions for common tasks related to the operating system                    |
| `io`        | Basic wrapper for general purpose input/output operations                        |
| `fs`        | File system operations extending functionality of the `io` module                |
| `net`       | Module for networking capabilities                                               |
| `vec`       | Vector data type implemented as growable array                                   |
| `bootstrap` | Module required when compiling applications                                      |

## Bootstrap

require dependency library in source
    library name
mark function as external library source
